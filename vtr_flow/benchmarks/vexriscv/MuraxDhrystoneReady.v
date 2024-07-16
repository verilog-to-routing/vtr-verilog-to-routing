// Generator : SpinalHDL v1.8.0    git head : 4e3563a282582b41f4eaafc503787757251d23ea
// Component : MuraxDhrystoneReady
// Git hash  : 51b69a1527c01616f386fa5cffb993313bfec919

`timescale 1ns/1ps

module Murax (
  input               io_asyncReset,
  input               io_mainClk,
  input               io_jtag_tms,
  input               io_jtag_tdi,
  output              io_jtag_tdo,
  input               io_jtag_tck,
  input      [31:0]   io_gpioA_read,
  output     [31:0]   io_gpioA_write,
  output     [31:0]   io_gpioA_writeEnable,
  output              io_uart_txd,
  input               io_uart_rxd
);

  wire       [7:0]    system_cpu_debug_bus_cmd_payload_address;
  wire                system_cpu_dBus_cmd_ready;
  reg                 system_ram_io_bus_cmd_valid;
  reg                 system_apbBridge_io_pipelinedMemoryBus_cmd_valid;
  wire       [3:0]    system_gpioACtrl_io_apb_PADDR;
  wire       [4:0]    system_uartCtrl_io_apb_PADDR;
  wire       [7:0]    system_timer_io_apb_PADDR;
  wire                io_asyncReset_buffercc_io_dataOut;
  wire                system_mainBusArbiter_io_iBus_cmd_ready;
  wire                system_mainBusArbiter_io_iBus_rsp_valid;
  wire                system_mainBusArbiter_io_iBus_rsp_payload_error;
  wire       [31:0]   system_mainBusArbiter_io_iBus_rsp_payload_inst;
  wire                system_mainBusArbiter_io_dBus_cmd_ready;
  wire                system_mainBusArbiter_io_dBus_rsp_ready;
  wire                system_mainBusArbiter_io_dBus_rsp_error;
  wire       [31:0]   system_mainBusArbiter_io_dBus_rsp_data;
  wire                system_mainBusArbiter_io_masterBus_cmd_valid;
  wire                system_mainBusArbiter_io_masterBus_cmd_payload_write;
  wire       [31:0]   system_mainBusArbiter_io_masterBus_cmd_payload_address;
  wire       [31:0]   system_mainBusArbiter_io_masterBus_cmd_payload_data;
  wire       [3:0]    system_mainBusArbiter_io_masterBus_cmd_payload_mask;
  wire                system_cpu_iBus_cmd_valid;
  wire       [31:0]   system_cpu_iBus_cmd_payload_pc;
  wire                system_cpu_debug_bus_cmd_ready;
  wire       [31:0]   system_cpu_debug_bus_rsp_data;
  wire                system_cpu_debug_resetOut;
  wire                system_cpu_dBus_cmd_valid;
  wire                system_cpu_dBus_cmd_payload_wr;
  wire       [31:0]   system_cpu_dBus_cmd_payload_address;
  wire       [31:0]   system_cpu_dBus_cmd_payload_data;
  wire       [1:0]    system_cpu_dBus_cmd_payload_size;
  wire                jtagBridge_1_io_jtag_tdo;
  wire                jtagBridge_1_io_remote_cmd_valid;
  wire                jtagBridge_1_io_remote_cmd_payload_last;
  wire       [0:0]    jtagBridge_1_io_remote_cmd_payload_fragment;
  wire                jtagBridge_1_io_remote_rsp_ready;
  wire                systemDebugger_1_io_remote_cmd_ready;
  wire                systemDebugger_1_io_remote_rsp_valid;
  wire                systemDebugger_1_io_remote_rsp_payload_error;
  wire       [31:0]   systemDebugger_1_io_remote_rsp_payload_data;
  wire                systemDebugger_1_io_mem_cmd_valid;
  wire       [31:0]   systemDebugger_1_io_mem_cmd_payload_address;
  wire       [31:0]   systemDebugger_1_io_mem_cmd_payload_data;
  wire                systemDebugger_1_io_mem_cmd_payload_wr;
  wire       [1:0]    systemDebugger_1_io_mem_cmd_payload_size;
  wire                system_ram_io_bus_cmd_ready;
  wire                system_ram_io_bus_rsp_valid;
  wire       [31:0]   system_ram_io_bus_rsp_payload_data;
  wire                system_apbBridge_io_pipelinedMemoryBus_cmd_ready;
  wire                system_apbBridge_io_pipelinedMemoryBus_rsp_valid;
  wire       [31:0]   system_apbBridge_io_pipelinedMemoryBus_rsp_payload_data;
  wire       [19:0]   system_apbBridge_io_apb_PADDR;
  wire       [0:0]    system_apbBridge_io_apb_PSEL;
  wire                system_apbBridge_io_apb_PENABLE;
  wire                system_apbBridge_io_apb_PWRITE;
  wire       [31:0]   system_apbBridge_io_apb_PWDATA;
  wire                system_gpioACtrl_io_apb_PREADY;
  wire       [31:0]   system_gpioACtrl_io_apb_PRDATA;
  wire                system_gpioACtrl_io_apb_PSLVERROR;
  wire       [31:0]   system_gpioACtrl_io_gpio_write;
  wire       [31:0]   system_gpioACtrl_io_gpio_writeEnable;
  wire       [31:0]   system_gpioACtrl_io_value;
  wire                system_uartCtrl_io_apb_PREADY;
  wire       [31:0]   system_uartCtrl_io_apb_PRDATA;
  wire                system_uartCtrl_io_uart_txd;
  wire                system_uartCtrl_io_interrupt;
  wire                system_timer_io_apb_PREADY;
  wire       [31:0]   system_timer_io_apb_PRDATA;
  wire                system_timer_io_apb_PSLVERROR;
  wire                system_timer_io_interrupt;
  wire                io_apb_decoder_io_input_PREADY;
  wire       [31:0]   io_apb_decoder_io_input_PRDATA;
  wire                io_apb_decoder_io_input_PSLVERROR;
  wire       [19:0]   io_apb_decoder_io_output_PADDR;
  wire       [2:0]    io_apb_decoder_io_output_PSEL;
  wire                io_apb_decoder_io_output_PENABLE;
  wire                io_apb_decoder_io_output_PWRITE;
  wire       [31:0]   io_apb_decoder_io_output_PWDATA;
  wire                apb3Router_1_io_input_PREADY;
  wire       [31:0]   apb3Router_1_io_input_PRDATA;
  wire                apb3Router_1_io_input_PSLVERROR;
  wire       [19:0]   apb3Router_1_io_outputs_0_PADDR;
  wire       [0:0]    apb3Router_1_io_outputs_0_PSEL;
  wire                apb3Router_1_io_outputs_0_PENABLE;
  wire                apb3Router_1_io_outputs_0_PWRITE;
  wire       [31:0]   apb3Router_1_io_outputs_0_PWDATA;
  wire       [19:0]   apb3Router_1_io_outputs_1_PADDR;
  wire       [0:0]    apb3Router_1_io_outputs_1_PSEL;
  wire                apb3Router_1_io_outputs_1_PENABLE;
  wire                apb3Router_1_io_outputs_1_PWRITE;
  wire       [31:0]   apb3Router_1_io_outputs_1_PWDATA;
  wire       [19:0]   apb3Router_1_io_outputs_2_PADDR;
  wire       [0:0]    apb3Router_1_io_outputs_2_PSEL;
  wire                apb3Router_1_io_outputs_2_PENABLE;
  wire                apb3Router_1_io_outputs_2_PWRITE;
  wire       [31:0]   apb3Router_1_io_outputs_2_PWDATA;
  reg        [31:0]   _zz_system_mainBusDecoder_logic_masterPipelined_rsp_payload_data;
  reg                 resetCtrl_mainClkResetUnbuffered;
  reg        [5:0]    resetCtrl_systemClkResetCounter;
  wire       [5:0]    _zz_when_Murax_l188;
  wire                when_Murax_l188;
  wire                when_Murax_l192;
  reg                 resetCtrl_mainClkReset;
  reg                 resetCtrl_systemReset;
  reg                 system_timerInterrupt;
  reg                 system_externalInterrupt;
  wire                toplevel_system_cpu_dBus_cmd_halfPipe_valid;
  wire                toplevel_system_cpu_dBus_cmd_halfPipe_ready;
  wire                toplevel_system_cpu_dBus_cmd_halfPipe_payload_wr;
  wire       [31:0]   toplevel_system_cpu_dBus_cmd_halfPipe_payload_address;
  wire       [31:0]   toplevel_system_cpu_dBus_cmd_halfPipe_payload_data;
  wire       [1:0]    toplevel_system_cpu_dBus_cmd_halfPipe_payload_size;
  reg                 toplevel_system_cpu_dBus_cmd_rValid;
  wire                toplevel_system_cpu_dBus_cmd_halfPipe_fire;
  reg                 toplevel_system_cpu_dBus_cmd_rData_wr;
  reg        [31:0]   toplevel_system_cpu_dBus_cmd_rData_address;
  reg        [31:0]   toplevel_system_cpu_dBus_cmd_rData_data;
  reg        [1:0]    toplevel_system_cpu_dBus_cmd_rData_size;
  reg                 toplevel_system_cpu_debug_resetOut_regNext;
  wire                toplevel_system_cpu_debug_bus_cmd_fire;
  reg                 toplevel_system_cpu_debug_bus_cmd_fire_regNext;
  wire                system_mainBusDecoder_logic_masterPipelined_cmd_valid;
  reg                 system_mainBusDecoder_logic_masterPipelined_cmd_ready;
  wire                system_mainBusDecoder_logic_masterPipelined_cmd_payload_write;
  wire       [31:0]   system_mainBusDecoder_logic_masterPipelined_cmd_payload_address;
  wire       [31:0]   system_mainBusDecoder_logic_masterPipelined_cmd_payload_data;
  wire       [3:0]    system_mainBusDecoder_logic_masterPipelined_cmd_payload_mask;
  wire                system_mainBusDecoder_logic_masterPipelined_rsp_valid;
  wire       [31:0]   system_mainBusDecoder_logic_masterPipelined_rsp_payload_data;
  wire                system_mainBusDecoder_logic_hits_0;
  wire                _zz_io_bus_cmd_payload_write;
  wire                system_mainBusDecoder_logic_hits_1;
  wire                _zz_io_pipelinedMemoryBus_cmd_payload_write;
  wire                system_mainBusDecoder_logic_noHit;
  reg                 system_mainBusDecoder_logic_rspPending;
  wire                system_mainBusDecoder_logic_masterPipelined_cmd_fire;
  wire                when_MuraxUtiles_l127;
  reg                 system_mainBusDecoder_logic_rspNoHit;
  wire                system_mainBusDecoder_logic_masterPipelined_cmd_fire_1;
  reg        [0:0]    system_mainBusDecoder_logic_rspSourceId;
  wire                when_MuraxUtiles_l133;

  BufferCC io_asyncReset_buffercc (
    .io_dataIn  (io_asyncReset                    ), //i
    .io_dataOut (io_asyncReset_buffercc_io_dataOut), //o
    .io_mainClk (io_mainClk                       )  //i
  );
  MuraxMasterArbiter system_mainBusArbiter (
    .io_iBus_cmd_valid                (system_cpu_iBus_cmd_valid                                         ), //i
    .io_iBus_cmd_ready                (system_mainBusArbiter_io_iBus_cmd_ready                           ), //o
    .io_iBus_cmd_payload_pc           (system_cpu_iBus_cmd_payload_pc[31:0]                              ), //i
    .io_iBus_rsp_valid                (system_mainBusArbiter_io_iBus_rsp_valid                           ), //o
    .io_iBus_rsp_payload_error        (system_mainBusArbiter_io_iBus_rsp_payload_error                   ), //o
    .io_iBus_rsp_payload_inst         (system_mainBusArbiter_io_iBus_rsp_payload_inst[31:0]              ), //o
    .io_dBus_cmd_valid                (toplevel_system_cpu_dBus_cmd_halfPipe_valid                       ), //i
    .io_dBus_cmd_ready                (system_mainBusArbiter_io_dBus_cmd_ready                           ), //o
    .io_dBus_cmd_payload_wr           (toplevel_system_cpu_dBus_cmd_halfPipe_payload_wr                  ), //i
    .io_dBus_cmd_payload_address      (toplevel_system_cpu_dBus_cmd_halfPipe_payload_address[31:0]       ), //i
    .io_dBus_cmd_payload_data         (toplevel_system_cpu_dBus_cmd_halfPipe_payload_data[31:0]          ), //i
    .io_dBus_cmd_payload_size         (toplevel_system_cpu_dBus_cmd_halfPipe_payload_size[1:0]           ), //i
    .io_dBus_rsp_ready                (system_mainBusArbiter_io_dBus_rsp_ready                           ), //o
    .io_dBus_rsp_error                (system_mainBusArbiter_io_dBus_rsp_error                           ), //o
    .io_dBus_rsp_data                 (system_mainBusArbiter_io_dBus_rsp_data[31:0]                      ), //o
    .io_masterBus_cmd_valid           (system_mainBusArbiter_io_masterBus_cmd_valid                      ), //o
    .io_masterBus_cmd_ready           (system_mainBusDecoder_logic_masterPipelined_cmd_ready             ), //i
    .io_masterBus_cmd_payload_write   (system_mainBusArbiter_io_masterBus_cmd_payload_write              ), //o
    .io_masterBus_cmd_payload_address (system_mainBusArbiter_io_masterBus_cmd_payload_address[31:0]      ), //o
    .io_masterBus_cmd_payload_data    (system_mainBusArbiter_io_masterBus_cmd_payload_data[31:0]         ), //o
    .io_masterBus_cmd_payload_mask    (system_mainBusArbiter_io_masterBus_cmd_payload_mask[3:0]          ), //o
    .io_masterBus_rsp_valid           (system_mainBusDecoder_logic_masterPipelined_rsp_valid             ), //i
    .io_masterBus_rsp_payload_data    (system_mainBusDecoder_logic_masterPipelined_rsp_payload_data[31:0]), //i
    .io_mainClk                       (io_mainClk                                                        ), //i
    .resetCtrl_systemReset            (resetCtrl_systemReset                                             )  //i
  );
  VexRiscv system_cpu (
    .iBus_cmd_valid                (system_cpu_iBus_cmd_valid                           ), //o
    .iBus_cmd_ready                (system_mainBusArbiter_io_iBus_cmd_ready             ), //i
    .iBus_cmd_payload_pc           (system_cpu_iBus_cmd_payload_pc[31:0]                ), //o
    .iBus_rsp_valid                (system_mainBusArbiter_io_iBus_rsp_valid             ), //i
    .iBus_rsp_payload_error        (system_mainBusArbiter_io_iBus_rsp_payload_error     ), //i
    .iBus_rsp_payload_inst         (system_mainBusArbiter_io_iBus_rsp_payload_inst[31:0]), //i
    .timerInterrupt                (system_timerInterrupt                               ), //i
    .externalInterrupt             (system_externalInterrupt                            ), //i
    .softwareInterrupt             (1'b0                                                ), //i
    .debug_bus_cmd_valid           (systemDebugger_1_io_mem_cmd_valid                   ), //i
    .debug_bus_cmd_ready           (system_cpu_debug_bus_cmd_ready                      ), //o
    .debug_bus_cmd_payload_wr      (systemDebugger_1_io_mem_cmd_payload_wr              ), //i
    .debug_bus_cmd_payload_address (system_cpu_debug_bus_cmd_payload_address[7:0]       ), //i
    .debug_bus_cmd_payload_data    (systemDebugger_1_io_mem_cmd_payload_data[31:0]      ), //i
    .debug_bus_rsp_data            (system_cpu_debug_bus_rsp_data[31:0]                 ), //o
    .debug_resetOut                (system_cpu_debug_resetOut                           ), //o
    .dBus_cmd_valid                (system_cpu_dBus_cmd_valid                           ), //o
    .dBus_cmd_ready                (system_cpu_dBus_cmd_ready                           ), //i
    .dBus_cmd_payload_wr           (system_cpu_dBus_cmd_payload_wr                      ), //o
    .dBus_cmd_payload_address      (system_cpu_dBus_cmd_payload_address[31:0]           ), //o
    .dBus_cmd_payload_data         (system_cpu_dBus_cmd_payload_data[31:0]              ), //o
    .dBus_cmd_payload_size         (system_cpu_dBus_cmd_payload_size[1:0]               ), //o
    .dBus_rsp_ready                (system_mainBusArbiter_io_dBus_rsp_ready             ), //i
    .dBus_rsp_error                (system_mainBusArbiter_io_dBus_rsp_error             ), //i
    .dBus_rsp_data                 (system_mainBusArbiter_io_dBus_rsp_data[31:0]        ), //i
    .io_mainClk                    (io_mainClk                                          ), //i
    .resetCtrl_systemReset         (resetCtrl_systemReset                               ), //i
    .resetCtrl_mainClkReset        (resetCtrl_mainClkReset                              )  //i
  );
  JtagBridge jtagBridge_1 (
    .io_jtag_tms                    (io_jtag_tms                                      ), //i
    .io_jtag_tdi                    (io_jtag_tdi                                      ), //i
    .io_jtag_tdo                    (jtagBridge_1_io_jtag_tdo                         ), //o
    .io_jtag_tck                    (io_jtag_tck                                      ), //i
    .io_remote_cmd_valid            (jtagBridge_1_io_remote_cmd_valid                 ), //o
    .io_remote_cmd_ready            (systemDebugger_1_io_remote_cmd_ready             ), //i
    .io_remote_cmd_payload_last     (jtagBridge_1_io_remote_cmd_payload_last          ), //o
    .io_remote_cmd_payload_fragment (jtagBridge_1_io_remote_cmd_payload_fragment      ), //o
    .io_remote_rsp_valid            (systemDebugger_1_io_remote_rsp_valid             ), //i
    .io_remote_rsp_ready            (jtagBridge_1_io_remote_rsp_ready                 ), //o
    .io_remote_rsp_payload_error    (systemDebugger_1_io_remote_rsp_payload_error     ), //i
    .io_remote_rsp_payload_data     (systemDebugger_1_io_remote_rsp_payload_data[31:0]), //i
    .io_mainClk                     (io_mainClk                                       ), //i
    .resetCtrl_mainClkReset         (resetCtrl_mainClkReset                           )  //i
  );
  SystemDebugger systemDebugger_1 (
    .io_remote_cmd_valid            (jtagBridge_1_io_remote_cmd_valid                 ), //i
    .io_remote_cmd_ready            (systemDebugger_1_io_remote_cmd_ready             ), //o
    .io_remote_cmd_payload_last     (jtagBridge_1_io_remote_cmd_payload_last          ), //i
    .io_remote_cmd_payload_fragment (jtagBridge_1_io_remote_cmd_payload_fragment      ), //i
    .io_remote_rsp_valid            (systemDebugger_1_io_remote_rsp_valid             ), //o
    .io_remote_rsp_ready            (jtagBridge_1_io_remote_rsp_ready                 ), //i
    .io_remote_rsp_payload_error    (systemDebugger_1_io_remote_rsp_payload_error     ), //o
    .io_remote_rsp_payload_data     (systemDebugger_1_io_remote_rsp_payload_data[31:0]), //o
    .io_mem_cmd_valid               (systemDebugger_1_io_mem_cmd_valid                ), //o
    .io_mem_cmd_ready               (system_cpu_debug_bus_cmd_ready                   ), //i
    .io_mem_cmd_payload_address     (systemDebugger_1_io_mem_cmd_payload_address[31:0]), //o
    .io_mem_cmd_payload_data        (systemDebugger_1_io_mem_cmd_payload_data[31:0]   ), //o
    .io_mem_cmd_payload_wr          (systemDebugger_1_io_mem_cmd_payload_wr           ), //o
    .io_mem_cmd_payload_size        (systemDebugger_1_io_mem_cmd_payload_size[1:0]    ), //o
    .io_mem_rsp_valid               (toplevel_system_cpu_debug_bus_cmd_fire_regNext   ), //i
    .io_mem_rsp_payload             (system_cpu_debug_bus_rsp_data[31:0]              ), //i
    .io_mainClk                     (io_mainClk                                       ), //i
    .resetCtrl_mainClkReset         (resetCtrl_mainClkReset                           )  //i
  );
  MuraxPipelinedMemoryBusRam system_ram (
    .io_bus_cmd_valid           (system_ram_io_bus_cmd_valid                                          ), //i
    .io_bus_cmd_ready           (system_ram_io_bus_cmd_ready                                          ), //o
    .io_bus_cmd_payload_write   (_zz_io_bus_cmd_payload_write                                         ), //i
    .io_bus_cmd_payload_address (system_mainBusDecoder_logic_masterPipelined_cmd_payload_address[31:0]), //i
    .io_bus_cmd_payload_data    (system_mainBusDecoder_logic_masterPipelined_cmd_payload_data[31:0]   ), //i
    .io_bus_cmd_payload_mask    (system_mainBusDecoder_logic_masterPipelined_cmd_payload_mask[3:0]    ), //i
    .io_bus_rsp_valid           (system_ram_io_bus_rsp_valid                                          ), //o
    .io_bus_rsp_payload_data    (system_ram_io_bus_rsp_payload_data[31:0]                             ), //o
    .io_mainClk                 (io_mainClk                                                           ), //i
    .resetCtrl_systemReset      (resetCtrl_systemReset                                                )  //i
  );
  PipelinedMemoryBusToApbBridge system_apbBridge (
    .io_pipelinedMemoryBus_cmd_valid           (system_apbBridge_io_pipelinedMemoryBus_cmd_valid                     ), //i
    .io_pipelinedMemoryBus_cmd_ready           (system_apbBridge_io_pipelinedMemoryBus_cmd_ready                     ), //o
    .io_pipelinedMemoryBus_cmd_payload_write   (_zz_io_pipelinedMemoryBus_cmd_payload_write                          ), //i
    .io_pipelinedMemoryBus_cmd_payload_address (system_mainBusDecoder_logic_masterPipelined_cmd_payload_address[31:0]), //i
    .io_pipelinedMemoryBus_cmd_payload_data    (system_mainBusDecoder_logic_masterPipelined_cmd_payload_data[31:0]   ), //i
    .io_pipelinedMemoryBus_cmd_payload_mask    (system_mainBusDecoder_logic_masterPipelined_cmd_payload_mask[3:0]    ), //i
    .io_pipelinedMemoryBus_rsp_valid           (system_apbBridge_io_pipelinedMemoryBus_rsp_valid                     ), //o
    .io_pipelinedMemoryBus_rsp_payload_data    (system_apbBridge_io_pipelinedMemoryBus_rsp_payload_data[31:0]        ), //o
    .io_apb_PADDR                              (system_apbBridge_io_apb_PADDR[19:0]                                  ), //o
    .io_apb_PSEL                               (system_apbBridge_io_apb_PSEL                                         ), //o
    .io_apb_PENABLE                            (system_apbBridge_io_apb_PENABLE                                      ), //o
    .io_apb_PREADY                             (io_apb_decoder_io_input_PREADY                                       ), //i
    .io_apb_PWRITE                             (system_apbBridge_io_apb_PWRITE                                       ), //o
    .io_apb_PWDATA                             (system_apbBridge_io_apb_PWDATA[31:0]                                 ), //o
    .io_apb_PRDATA                             (io_apb_decoder_io_input_PRDATA[31:0]                                 ), //i
    .io_apb_PSLVERROR                          (io_apb_decoder_io_input_PSLVERROR                                    ), //i
    .io_mainClk                                (io_mainClk                                                           ), //i
    .resetCtrl_systemReset                     (resetCtrl_systemReset                                                )  //i
  );
  Apb3Gpio system_gpioACtrl (
    .io_apb_PADDR          (system_gpioACtrl_io_apb_PADDR[3:0]        ), //i
    .io_apb_PSEL           (apb3Router_1_io_outputs_0_PSEL            ), //i
    .io_apb_PENABLE        (apb3Router_1_io_outputs_0_PENABLE         ), //i
    .io_apb_PREADY         (system_gpioACtrl_io_apb_PREADY            ), //o
    .io_apb_PWRITE         (apb3Router_1_io_outputs_0_PWRITE          ), //i
    .io_apb_PWDATA         (apb3Router_1_io_outputs_0_PWDATA[31:0]    ), //i
    .io_apb_PRDATA         (system_gpioACtrl_io_apb_PRDATA[31:0]      ), //o
    .io_apb_PSLVERROR      (system_gpioACtrl_io_apb_PSLVERROR         ), //o
    .io_gpio_read          (io_gpioA_read[31:0]                       ), //i
    .io_gpio_write         (system_gpioACtrl_io_gpio_write[31:0]      ), //o
    .io_gpio_writeEnable   (system_gpioACtrl_io_gpio_writeEnable[31:0]), //o
    .io_value              (system_gpioACtrl_io_value[31:0]           ), //o
    .io_mainClk            (io_mainClk                                ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                     )  //i
  );
  Apb3UartCtrl system_uartCtrl (
    .io_apb_PADDR          (system_uartCtrl_io_apb_PADDR[4:0]     ), //i
    .io_apb_PSEL           (apb3Router_1_io_outputs_1_PSEL        ), //i
    .io_apb_PENABLE        (apb3Router_1_io_outputs_1_PENABLE     ), //i
    .io_apb_PREADY         (system_uartCtrl_io_apb_PREADY         ), //o
    .io_apb_PWRITE         (apb3Router_1_io_outputs_1_PWRITE      ), //i
    .io_apb_PWDATA         (apb3Router_1_io_outputs_1_PWDATA[31:0]), //i
    .io_apb_PRDATA         (system_uartCtrl_io_apb_PRDATA[31:0]   ), //o
    .io_uart_txd           (system_uartCtrl_io_uart_txd           ), //o
    .io_uart_rxd           (io_uart_rxd                           ), //i
    .io_interrupt          (system_uartCtrl_io_interrupt          ), //o
    .io_mainClk            (io_mainClk                            ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                 )  //i
  );
  MuraxApb3Timer system_timer (
    .io_apb_PADDR          (system_timer_io_apb_PADDR[7:0]        ), //i
    .io_apb_PSEL           (apb3Router_1_io_outputs_2_PSEL        ), //i
    .io_apb_PENABLE        (apb3Router_1_io_outputs_2_PENABLE     ), //i
    .io_apb_PREADY         (system_timer_io_apb_PREADY            ), //o
    .io_apb_PWRITE         (apb3Router_1_io_outputs_2_PWRITE      ), //i
    .io_apb_PWDATA         (apb3Router_1_io_outputs_2_PWDATA[31:0]), //i
    .io_apb_PRDATA         (system_timer_io_apb_PRDATA[31:0]      ), //o
    .io_apb_PSLVERROR      (system_timer_io_apb_PSLVERROR         ), //o
    .io_interrupt          (system_timer_io_interrupt             ), //o
    .io_mainClk            (io_mainClk                            ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                 )  //i
  );
  Apb3Decoder io_apb_decoder (
    .io_input_PADDR      (system_apbBridge_io_apb_PADDR[19:0]  ), //i
    .io_input_PSEL       (system_apbBridge_io_apb_PSEL         ), //i
    .io_input_PENABLE    (system_apbBridge_io_apb_PENABLE      ), //i
    .io_input_PREADY     (io_apb_decoder_io_input_PREADY       ), //o
    .io_input_PWRITE     (system_apbBridge_io_apb_PWRITE       ), //i
    .io_input_PWDATA     (system_apbBridge_io_apb_PWDATA[31:0] ), //i
    .io_input_PRDATA     (io_apb_decoder_io_input_PRDATA[31:0] ), //o
    .io_input_PSLVERROR  (io_apb_decoder_io_input_PSLVERROR    ), //o
    .io_output_PADDR     (io_apb_decoder_io_output_PADDR[19:0] ), //o
    .io_output_PSEL      (io_apb_decoder_io_output_PSEL[2:0]   ), //o
    .io_output_PENABLE   (io_apb_decoder_io_output_PENABLE     ), //o
    .io_output_PREADY    (apb3Router_1_io_input_PREADY         ), //i
    .io_output_PWRITE    (io_apb_decoder_io_output_PWRITE      ), //o
    .io_output_PWDATA    (io_apb_decoder_io_output_PWDATA[31:0]), //o
    .io_output_PRDATA    (apb3Router_1_io_input_PRDATA[31:0]   ), //i
    .io_output_PSLVERROR (apb3Router_1_io_input_PSLVERROR      )  //i
  );
  Apb3Router apb3Router_1 (
    .io_input_PADDR         (io_apb_decoder_io_output_PADDR[19:0]  ), //i
    .io_input_PSEL          (io_apb_decoder_io_output_PSEL[2:0]    ), //i
    .io_input_PENABLE       (io_apb_decoder_io_output_PENABLE      ), //i
    .io_input_PREADY        (apb3Router_1_io_input_PREADY          ), //o
    .io_input_PWRITE        (io_apb_decoder_io_output_PWRITE       ), //i
    .io_input_PWDATA        (io_apb_decoder_io_output_PWDATA[31:0] ), //i
    .io_input_PRDATA        (apb3Router_1_io_input_PRDATA[31:0]    ), //o
    .io_input_PSLVERROR     (apb3Router_1_io_input_PSLVERROR       ), //o
    .io_outputs_0_PADDR     (apb3Router_1_io_outputs_0_PADDR[19:0] ), //o
    .io_outputs_0_PSEL      (apb3Router_1_io_outputs_0_PSEL        ), //o
    .io_outputs_0_PENABLE   (apb3Router_1_io_outputs_0_PENABLE     ), //o
    .io_outputs_0_PREADY    (system_gpioACtrl_io_apb_PREADY        ), //i
    .io_outputs_0_PWRITE    (apb3Router_1_io_outputs_0_PWRITE      ), //o
    .io_outputs_0_PWDATA    (apb3Router_1_io_outputs_0_PWDATA[31:0]), //o
    .io_outputs_0_PRDATA    (system_gpioACtrl_io_apb_PRDATA[31:0]  ), //i
    .io_outputs_0_PSLVERROR (system_gpioACtrl_io_apb_PSLVERROR     ), //i
    .io_outputs_1_PADDR     (apb3Router_1_io_outputs_1_PADDR[19:0] ), //o
    .io_outputs_1_PSEL      (apb3Router_1_io_outputs_1_PSEL        ), //o
    .io_outputs_1_PENABLE   (apb3Router_1_io_outputs_1_PENABLE     ), //o
    .io_outputs_1_PREADY    (system_uartCtrl_io_apb_PREADY         ), //i
    .io_outputs_1_PWRITE    (apb3Router_1_io_outputs_1_PWRITE      ), //o
    .io_outputs_1_PWDATA    (apb3Router_1_io_outputs_1_PWDATA[31:0]), //o
    .io_outputs_1_PRDATA    (system_uartCtrl_io_apb_PRDATA[31:0]   ), //i
    .io_outputs_1_PSLVERROR (1'b0                                  ), //i
    .io_outputs_2_PADDR     (apb3Router_1_io_outputs_2_PADDR[19:0] ), //o
    .io_outputs_2_PSEL      (apb3Router_1_io_outputs_2_PSEL        ), //o
    .io_outputs_2_PENABLE   (apb3Router_1_io_outputs_2_PENABLE     ), //o
    .io_outputs_2_PREADY    (system_timer_io_apb_PREADY            ), //i
    .io_outputs_2_PWRITE    (apb3Router_1_io_outputs_2_PWRITE      ), //o
    .io_outputs_2_PWDATA    (apb3Router_1_io_outputs_2_PWDATA[31:0]), //o
    .io_outputs_2_PRDATA    (system_timer_io_apb_PRDATA[31:0]      ), //i
    .io_outputs_2_PSLVERROR (system_timer_io_apb_PSLVERROR         ), //i
    .io_mainClk             (io_mainClk                            ), //i
    .resetCtrl_systemReset  (resetCtrl_systemReset                 )  //i
  );
  initial begin
    resetCtrl_systemClkResetCounter = 6'h0;
  end

  always @(*) begin
    case(system_mainBusDecoder_logic_rspSourceId)
      1'b0 : _zz_system_mainBusDecoder_logic_masterPipelined_rsp_payload_data = system_ram_io_bus_rsp_payload_data;
      default : _zz_system_mainBusDecoder_logic_masterPipelined_rsp_payload_data = system_apbBridge_io_pipelinedMemoryBus_rsp_payload_data;
    endcase
  end

  always @(*) begin
    resetCtrl_mainClkResetUnbuffered = 1'b0; // @[Murax.scala 183:35]
    if(when_Murax_l188) begin
      resetCtrl_mainClkResetUnbuffered = 1'b1; // @[Murax.scala 190:30]
    end
  end

  assign _zz_when_Murax_l188[5 : 0] = 6'h3f; // @[Literal.scala 88:56]
  assign when_Murax_l188 = (resetCtrl_systemClkResetCounter != _zz_when_Murax_l188); // @[BaseType.scala 305:24]
  assign when_Murax_l192 = io_asyncReset_buffercc_io_dataOut; // @[CrossClock.scala 13:9]
  always @(*) begin
    system_timerInterrupt = 1'b0; // @[Murax.scala 234:26]
    if(system_timer_io_interrupt) begin
      system_timerInterrupt = 1'b1; // @[Murax.scala 295:20]
    end
  end

  always @(*) begin
    system_externalInterrupt = 1'b0; // @[Murax.scala 235:29]
    if(system_uartCtrl_io_interrupt) begin
      system_externalInterrupt = 1'b1; // @[Murax.scala 291:23]
    end
  end

  assign toplevel_system_cpu_dBus_cmd_halfPipe_fire = (toplevel_system_cpu_dBus_cmd_halfPipe_valid && toplevel_system_cpu_dBus_cmd_halfPipe_ready); // @[BaseType.scala 305:24]
  assign system_cpu_dBus_cmd_ready = (! toplevel_system_cpu_dBus_cmd_rValid); // @[Stream.scala 414:16]
  assign toplevel_system_cpu_dBus_cmd_halfPipe_valid = toplevel_system_cpu_dBus_cmd_rValid; // @[Stream.scala 416:20]
  assign toplevel_system_cpu_dBus_cmd_halfPipe_payload_wr = toplevel_system_cpu_dBus_cmd_rData_wr; // @[Stream.scala 417:22]
  assign toplevel_system_cpu_dBus_cmd_halfPipe_payload_address = toplevel_system_cpu_dBus_cmd_rData_address; // @[Stream.scala 417:22]
  assign toplevel_system_cpu_dBus_cmd_halfPipe_payload_data = toplevel_system_cpu_dBus_cmd_rData_data; // @[Stream.scala 417:22]
  assign toplevel_system_cpu_dBus_cmd_halfPipe_payload_size = toplevel_system_cpu_dBus_cmd_rData_size; // @[Stream.scala 417:22]
  assign toplevel_system_cpu_dBus_cmd_halfPipe_ready = system_mainBusArbiter_io_dBus_cmd_ready; // @[Stream.scala 295:16]
  assign system_cpu_debug_bus_cmd_payload_address = systemDebugger_1_io_mem_cmd_payload_address[7:0]; // @[DebugPlugin.scala 119:24]
  assign toplevel_system_cpu_debug_bus_cmd_fire = (systemDebugger_1_io_mem_cmd_valid && system_cpu_debug_bus_cmd_ready); // @[BaseType.scala 305:24]
  assign io_jtag_tdo = jtagBridge_1_io_jtag_tdo; // @[Murax.scala 254:17]
  assign io_gpioA_write = system_gpioACtrl_io_gpio_write; // @[Murax.scala 286:14]
  assign io_gpioA_writeEnable = system_gpioACtrl_io_gpio_writeEnable; // @[Murax.scala 286:14]
  assign io_uart_txd = system_uartCtrl_io_uart_txd; // @[Murax.scala 290:22]
  assign system_gpioACtrl_io_apb_PADDR = apb3Router_1_io_outputs_0_PADDR[3:0]; // @[APB3.scala 72:18]
  assign system_uartCtrl_io_apb_PADDR = apb3Router_1_io_outputs_1_PADDR[4:0]; // @[APB3.scala 72:18]
  assign system_timer_io_apb_PADDR = apb3Router_1_io_outputs_2_PADDR[7:0]; // @[APB3.scala 72:18]
  assign system_mainBusDecoder_logic_masterPipelined_cmd_valid = system_mainBusArbiter_io_masterBus_cmd_valid; // @[Stream.scala 294:16]
  assign system_mainBusDecoder_logic_masterPipelined_cmd_payload_write = system_mainBusArbiter_io_masterBus_cmd_payload_write; // @[Stream.scala 296:18]
  assign system_mainBusDecoder_logic_masterPipelined_cmd_payload_address = system_mainBusArbiter_io_masterBus_cmd_payload_address; // @[Stream.scala 296:18]
  assign system_mainBusDecoder_logic_masterPipelined_cmd_payload_data = system_mainBusArbiter_io_masterBus_cmd_payload_data; // @[Stream.scala 296:18]
  assign system_mainBusDecoder_logic_masterPipelined_cmd_payload_mask = system_mainBusArbiter_io_masterBus_cmd_payload_mask; // @[Stream.scala 296:18]
  assign system_mainBusDecoder_logic_hits_0 = ((system_mainBusDecoder_logic_masterPipelined_cmd_payload_address & (~ 32'h0003ffff)) == 32'h80000000); // @[BaseType.scala 305:24]
  always @(*) begin
    system_ram_io_bus_cmd_valid = (system_mainBusDecoder_logic_masterPipelined_cmd_valid && system_mainBusDecoder_logic_hits_0); // @[MuraxUtiles.scala 120:26]
    if(when_MuraxUtiles_l133) begin
      system_ram_io_bus_cmd_valid = 1'b0; // @[MuraxUtiles.scala 135:36]
    end
  end

  assign _zz_io_bus_cmd_payload_write = system_mainBusDecoder_logic_masterPipelined_cmd_payload_write; // @[Data.scala 450:19]
  assign system_mainBusDecoder_logic_hits_1 = ((system_mainBusDecoder_logic_masterPipelined_cmd_payload_address & (~ 32'h000fffff)) == 32'hf0000000); // @[BaseType.scala 305:24]
  always @(*) begin
    system_apbBridge_io_pipelinedMemoryBus_cmd_valid = (system_mainBusDecoder_logic_masterPipelined_cmd_valid && system_mainBusDecoder_logic_hits_1); // @[MuraxUtiles.scala 120:26]
    if(when_MuraxUtiles_l133) begin
      system_apbBridge_io_pipelinedMemoryBus_cmd_valid = 1'b0; // @[MuraxUtiles.scala 135:36]
    end
  end

  assign _zz_io_pipelinedMemoryBus_cmd_payload_write = system_mainBusDecoder_logic_masterPipelined_cmd_payload_write; // @[Data.scala 450:19]
  assign system_mainBusDecoder_logic_noHit = (! ({system_mainBusDecoder_logic_hits_1,system_mainBusDecoder_logic_hits_0} != 2'b00)); // @[BaseType.scala 299:24]
  always @(*) begin
    system_mainBusDecoder_logic_masterPipelined_cmd_ready = (({(system_mainBusDecoder_logic_hits_1 && system_apbBridge_io_pipelinedMemoryBus_cmd_ready),(system_mainBusDecoder_logic_hits_0 && system_ram_io_bus_cmd_ready)} != 2'b00) || system_mainBusDecoder_logic_noHit); // @[MuraxUtiles.scala 125:29]
    if(when_MuraxUtiles_l133) begin
      system_mainBusDecoder_logic_masterPipelined_cmd_ready = 1'b0; // @[MuraxUtiles.scala 134:31]
    end
  end

  assign system_mainBusDecoder_logic_masterPipelined_cmd_fire = (system_mainBusDecoder_logic_masterPipelined_cmd_valid && system_mainBusDecoder_logic_masterPipelined_cmd_ready); // @[BaseType.scala 305:24]
  assign when_MuraxUtiles_l127 = (system_mainBusDecoder_logic_masterPipelined_cmd_fire && (! system_mainBusDecoder_logic_masterPipelined_cmd_payload_write)); // @[BaseType.scala 305:24]
  assign system_mainBusDecoder_logic_masterPipelined_cmd_fire_1 = (system_mainBusDecoder_logic_masterPipelined_cmd_valid && system_mainBusDecoder_logic_masterPipelined_cmd_ready); // @[BaseType.scala 305:24]
  assign system_mainBusDecoder_logic_masterPipelined_rsp_valid = (({system_apbBridge_io_pipelinedMemoryBus_rsp_valid,system_ram_io_bus_rsp_valid} != 2'b00) || (system_mainBusDecoder_logic_rspPending && system_mainBusDecoder_logic_rspNoHit)); // @[MuraxUtiles.scala 130:31]
  assign system_mainBusDecoder_logic_masterPipelined_rsp_payload_data = _zz_system_mainBusDecoder_logic_masterPipelined_rsp_payload_data; // @[MuraxUtiles.scala 131:31]
  assign when_MuraxUtiles_l133 = (system_mainBusDecoder_logic_rspPending && (! system_mainBusDecoder_logic_masterPipelined_rsp_valid)); // @[BaseType.scala 305:24]
  always @(posedge io_mainClk) begin
    if(when_Murax_l188) begin
      resetCtrl_systemClkResetCounter <= (resetCtrl_systemClkResetCounter + 6'h01); // @[Murax.scala 189:29]
    end
    if(when_Murax_l192) begin
      resetCtrl_systemClkResetCounter <= 6'h0; // @[Murax.scala 193:29]
    end
  end

  always @(posedge io_mainClk) begin
    resetCtrl_mainClkReset <= resetCtrl_mainClkResetUnbuffered; // @[Reg.scala 39:30]
    resetCtrl_systemReset <= resetCtrl_mainClkResetUnbuffered; // @[Reg.scala 39:30]
    if(toplevel_system_cpu_debug_resetOut_regNext) begin
      resetCtrl_systemReset <= 1'b1; // @[Murax.scala 253:31]
    end
  end

  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      toplevel_system_cpu_dBus_cmd_rValid <= 1'b0; // @[Data.scala 400:33]
      system_mainBusDecoder_logic_rspPending <= 1'b0; // @[Data.scala 400:33]
      system_mainBusDecoder_logic_rspNoHit <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(system_cpu_dBus_cmd_valid) begin
        toplevel_system_cpu_dBus_cmd_rValid <= 1'b1; // @[Stream.scala 411:33]
      end
      if(toplevel_system_cpu_dBus_cmd_halfPipe_fire) begin
        toplevel_system_cpu_dBus_cmd_rValid <= 1'b0; // @[Stream.scala 411:53]
      end
      if(system_mainBusDecoder_logic_masterPipelined_rsp_valid) begin
        system_mainBusDecoder_logic_rspPending <= 1'b0; // @[MuraxUtiles.scala 127:36]
      end
      if(when_MuraxUtiles_l127) begin
        system_mainBusDecoder_logic_rspPending <= 1'b1; // @[MuraxUtiles.scala 127:73]
      end
      system_mainBusDecoder_logic_rspNoHit <= 1'b0; // @[Reg.scala 39:30]
      if(system_mainBusDecoder_logic_noHit) begin
        system_mainBusDecoder_logic_rspNoHit <= 1'b1; // @[MuraxUtiles.scala 128:48]
      end
    end
  end

  always @(posedge io_mainClk) begin
    if(system_cpu_dBus_cmd_ready) begin
      toplevel_system_cpu_dBus_cmd_rData_wr <= system_cpu_dBus_cmd_payload_wr; // @[Stream.scala 412:28]
      toplevel_system_cpu_dBus_cmd_rData_address <= system_cpu_dBus_cmd_payload_address; // @[Stream.scala 412:28]
      toplevel_system_cpu_dBus_cmd_rData_data <= system_cpu_dBus_cmd_payload_data; // @[Stream.scala 412:28]
      toplevel_system_cpu_dBus_cmd_rData_size <= system_cpu_dBus_cmd_payload_size; // @[Stream.scala 412:28]
    end
    if(system_mainBusDecoder_logic_masterPipelined_cmd_fire_1) begin
      system_mainBusDecoder_logic_rspSourceId <= system_mainBusDecoder_logic_hits_1; // @[MuraxUtiles.scala 129:32]
    end
  end

  always @(posedge io_mainClk) begin
    toplevel_system_cpu_debug_resetOut_regNext <= system_cpu_debug_resetOut; // @[Reg.scala 39:30]
  end

  always @(posedge io_mainClk or posedge resetCtrl_mainClkReset) begin
    if(resetCtrl_mainClkReset) begin
      toplevel_system_cpu_debug_bus_cmd_fire_regNext <= 1'b0; // @[Data.scala 400:33]
    end else begin
      toplevel_system_cpu_debug_bus_cmd_fire_regNext <= toplevel_system_cpu_debug_bus_cmd_fire; // @[Reg.scala 39:30]
    end
  end


endmodule

module Apb3Router (
  input      [19:0]   io_input_PADDR,
  input      [2:0]    io_input_PSEL,
  input               io_input_PENABLE,
  output              io_input_PREADY,
  input               io_input_PWRITE,
  input      [31:0]   io_input_PWDATA,
  output     [31:0]   io_input_PRDATA,
  output              io_input_PSLVERROR,
  output     [19:0]   io_outputs_0_PADDR,
  output     [0:0]    io_outputs_0_PSEL,
  output              io_outputs_0_PENABLE,
  input               io_outputs_0_PREADY,
  output              io_outputs_0_PWRITE,
  output     [31:0]   io_outputs_0_PWDATA,
  input      [31:0]   io_outputs_0_PRDATA,
  input               io_outputs_0_PSLVERROR,
  output     [19:0]   io_outputs_1_PADDR,
  output     [0:0]    io_outputs_1_PSEL,
  output              io_outputs_1_PENABLE,
  input               io_outputs_1_PREADY,
  output              io_outputs_1_PWRITE,
  output     [31:0]   io_outputs_1_PWDATA,
  input      [31:0]   io_outputs_1_PRDATA,
  input               io_outputs_1_PSLVERROR,
  output     [19:0]   io_outputs_2_PADDR,
  output     [0:0]    io_outputs_2_PSEL,
  output              io_outputs_2_PENABLE,
  input               io_outputs_2_PREADY,
  output              io_outputs_2_PWRITE,
  output     [31:0]   io_outputs_2_PWDATA,
  input      [31:0]   io_outputs_2_PRDATA,
  input               io_outputs_2_PSLVERROR,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg                 _zz_io_input_PREADY;
  reg        [31:0]   _zz_io_input_PRDATA;
  reg                 _zz_io_input_PSLVERROR;
  wire                _zz_selIndex;
  wire                _zz_selIndex_1;
  reg        [1:0]    selIndex;

  always @(*) begin
    case(selIndex)
      2'b00 : begin
        _zz_io_input_PREADY = io_outputs_0_PREADY;
        _zz_io_input_PRDATA = io_outputs_0_PRDATA;
        _zz_io_input_PSLVERROR = io_outputs_0_PSLVERROR;
      end
      2'b01 : begin
        _zz_io_input_PREADY = io_outputs_1_PREADY;
        _zz_io_input_PRDATA = io_outputs_1_PRDATA;
        _zz_io_input_PSLVERROR = io_outputs_1_PSLVERROR;
      end
      default : begin
        _zz_io_input_PREADY = io_outputs_2_PREADY;
        _zz_io_input_PRDATA = io_outputs_2_PRDATA;
        _zz_io_input_PSLVERROR = io_outputs_2_PSLVERROR;
      end
    endcase
  end

  assign io_outputs_0_PADDR = io_input_PADDR; // @[Apb3Router.scala 47:20]
  assign io_outputs_0_PENABLE = io_input_PENABLE; // @[Apb3Router.scala 48:20]
  assign io_outputs_0_PSEL[0] = io_input_PSEL[0]; // @[Apb3Router.scala 49:20]
  assign io_outputs_0_PWRITE = io_input_PWRITE; // @[Apb3Router.scala 50:20]
  assign io_outputs_0_PWDATA = io_input_PWDATA; // @[Apb3Router.scala 51:20]
  assign io_outputs_1_PADDR = io_input_PADDR; // @[Apb3Router.scala 47:20]
  assign io_outputs_1_PENABLE = io_input_PENABLE; // @[Apb3Router.scala 48:20]
  assign io_outputs_1_PSEL[0] = io_input_PSEL[1]; // @[Apb3Router.scala 49:20]
  assign io_outputs_1_PWRITE = io_input_PWRITE; // @[Apb3Router.scala 50:20]
  assign io_outputs_1_PWDATA = io_input_PWDATA; // @[Apb3Router.scala 51:20]
  assign io_outputs_2_PADDR = io_input_PADDR; // @[Apb3Router.scala 47:20]
  assign io_outputs_2_PENABLE = io_input_PENABLE; // @[Apb3Router.scala 48:20]
  assign io_outputs_2_PSEL[0] = io_input_PSEL[2]; // @[Apb3Router.scala 49:20]
  assign io_outputs_2_PWRITE = io_input_PWRITE; // @[Apb3Router.scala 50:20]
  assign io_outputs_2_PWDATA = io_input_PWDATA; // @[Apb3Router.scala 51:20]
  assign _zz_selIndex = io_input_PSEL[1]; // @[BaseType.scala 305:24]
  assign _zz_selIndex_1 = io_input_PSEL[2]; // @[BaseType.scala 305:24]
  assign io_input_PREADY = _zz_io_input_PREADY; // @[Apb3Router.scala 56:19]
  assign io_input_PRDATA = _zz_io_input_PRDATA; // @[Apb3Router.scala 57:19]
  assign io_input_PSLVERROR = _zz_io_input_PSLVERROR; // @[Apb3Router.scala 59:52]
  always @(posedge io_mainClk) begin
    selIndex <= {_zz_selIndex_1,_zz_selIndex}; // @[Reg.scala 39:30]
  end


endmodule

module Apb3Decoder (
  input      [19:0]   io_input_PADDR,
  input      [0:0]    io_input_PSEL,
  input               io_input_PENABLE,
  output reg          io_input_PREADY,
  input               io_input_PWRITE,
  input      [31:0]   io_input_PWDATA,
  output     [31:0]   io_input_PRDATA,
  output reg          io_input_PSLVERROR,
  output     [19:0]   io_output_PADDR,
  output reg [2:0]    io_output_PSEL,
  output              io_output_PENABLE,
  input               io_output_PREADY,
  output              io_output_PWRITE,
  output     [31:0]   io_output_PWDATA,
  input      [31:0]   io_output_PRDATA,
  input               io_output_PSLVERROR
);

  wire                when_Apb3Decoder_l88;

  assign io_output_PADDR = io_input_PADDR; // @[Apb3Decoder.scala 74:21]
  assign io_output_PENABLE = io_input_PENABLE; // @[Apb3Decoder.scala 75:21]
  assign io_output_PWRITE = io_input_PWRITE; // @[Apb3Decoder.scala 76:21]
  assign io_output_PWDATA = io_input_PWDATA; // @[Apb3Decoder.scala 77:21]
  always @(*) begin
    io_output_PSEL[0] = (((io_input_PADDR & (~ 20'h00fff)) == 20'h0) && io_input_PSEL[0]); // @[Apb3Decoder.scala 80:10]
    io_output_PSEL[1] = (((io_input_PADDR & (~ 20'h00fff)) == 20'h10000) && io_input_PSEL[0]); // @[Apb3Decoder.scala 80:10]
    io_output_PSEL[2] = (((io_input_PADDR & (~ 20'h00fff)) == 20'h20000) && io_input_PSEL[0]); // @[Apb3Decoder.scala 80:10]
  end

  always @(*) begin
    io_input_PREADY = io_output_PREADY; // @[Apb3Decoder.scala 83:19]
    if(when_Apb3Decoder_l88) begin
      io_input_PREADY = 1'b1; // @[Apb3Decoder.scala 89:21]
    end
  end

  assign io_input_PRDATA = io_output_PRDATA; // @[Apb3Decoder.scala 84:19]
  always @(*) begin
    io_input_PSLVERROR = io_output_PSLVERROR; // @[Apb3Decoder.scala 86:52]
    if(when_Apb3Decoder_l88) begin
      io_input_PSLVERROR = 1'b1; // @[Apb3Decoder.scala 90:54]
    end
  end

  assign when_Apb3Decoder_l88 = (io_input_PSEL[0] && (io_output_PSEL == 3'b000)); // @[BaseType.scala 305:24]

endmodule

module MuraxApb3Timer (
  input      [7:0]    io_apb_PADDR,
  input      [0:0]    io_apb_PSEL,
  input               io_apb_PENABLE,
  output              io_apb_PREADY,
  input               io_apb_PWRITE,
  input      [31:0]   io_apb_PWDATA,
  output reg [31:0]   io_apb_PRDATA,
  output              io_apb_PSLVERROR,
  output              io_interrupt,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  wire                timerA_io_tick;
  wire                timerA_io_clear;
  wire                timerB_io_tick;
  wire                timerB_io_clear;
  reg        [1:0]    interruptCtrl_1_io_inputs;
  reg        [1:0]    interruptCtrl_1_io_clears;
  wire                prescaler_1_io_overflow;
  wire                timerA_io_full;
  wire       [15:0]   timerA_io_value;
  wire                timerB_io_full;
  wire       [15:0]   timerB_io_value;
  wire       [1:0]    interruptCtrl_1_io_pendings;
  wire                busCtrl_readErrorFlag;
  wire                busCtrl_writeErrorFlag;
  wire                busCtrl_askWrite;
  wire                busCtrl_askRead;
  wire                busCtrl_doWrite;
  wire                busCtrl_doRead;
  reg        [15:0]   _zz_io_limit;
  reg                 _zz_io_clear;
  reg        [1:0]    timerABridge_ticksEnable;
  reg        [0:0]    timerABridge_clearsEnable;
  reg                 timerABridge_busClearing;
  reg        [15:0]   system_timer_timerA_io_limit_driver;
  reg                 when_Timer_l40;
  reg                 when_Timer_l44;
  reg        [1:0]    timerBBridge_ticksEnable;
  reg        [0:0]    timerBBridge_clearsEnable;
  reg                 timerBBridge_busClearing;
  reg        [15:0]   system_timer_timerB_io_limit_driver;
  reg                 when_Timer_l40_1;
  reg                 when_Timer_l44_1;
  reg        [1:0]    system_timer_interruptCtrl_1_io_masks_driver;

  Prescaler prescaler_1 (
    .io_clear              (_zz_io_clear           ), //i
    .io_limit              (_zz_io_limit[15:0]     ), //i
    .io_overflow           (prescaler_1_io_overflow), //o
    .io_mainClk            (io_mainClk             ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset  )  //i
  );
  Timer_1 timerA (
    .io_tick               (timerA_io_tick                           ), //i
    .io_clear              (timerA_io_clear                          ), //i
    .io_limit              (system_timer_timerA_io_limit_driver[15:0]), //i
    .io_full               (timerA_io_full                           ), //o
    .io_value              (timerA_io_value[15:0]                    ), //o
    .io_mainClk            (io_mainClk                               ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                    )  //i
  );
  Timer_1 timerB (
    .io_tick               (timerB_io_tick                           ), //i
    .io_clear              (timerB_io_clear                          ), //i
    .io_limit              (system_timer_timerB_io_limit_driver[15:0]), //i
    .io_full               (timerB_io_full                           ), //o
    .io_value              (timerB_io_value[15:0]                    ), //o
    .io_mainClk            (io_mainClk                               ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                    )  //i
  );
  InterruptCtrl interruptCtrl_1 (
    .io_inputs             (interruptCtrl_1_io_inputs[1:0]                   ), //i
    .io_clears             (interruptCtrl_1_io_clears[1:0]                   ), //i
    .io_masks              (system_timer_interruptCtrl_1_io_masks_driver[1:0]), //i
    .io_pendings           (interruptCtrl_1_io_pendings[1:0]                 ), //o
    .io_mainClk            (io_mainClk                                       ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                            )  //i
  );
  assign busCtrl_readErrorFlag = 1'b0; // @[BusSlaveFactory.scala 105:29]
  assign busCtrl_writeErrorFlag = 1'b0; // @[BusSlaveFactory.scala 106:30]
  assign io_apb_PREADY = 1'b1; // @[Apb3SlaveFactory.scala 38:14]
  always @(*) begin
    io_apb_PRDATA = 32'h0; // @[Apb3SlaveFactory.scala 39:68]
    case(io_apb_PADDR)
      8'h0 : begin
        io_apb_PRDATA[15 : 0] = _zz_io_limit; // @[BusSlaveFactory.scala 942:69]
      end
      8'h40 : begin
        io_apb_PRDATA[1 : 0] = timerABridge_ticksEnable; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[16 : 16] = timerABridge_clearsEnable; // @[BusSlaveFactory.scala 942:69]
      end
      8'h44 : begin
        io_apb_PRDATA[15 : 0] = system_timer_timerA_io_limit_driver; // @[BusSlaveFactory.scala 942:69]
      end
      8'h48 : begin
        io_apb_PRDATA[15 : 0] = timerA_io_value; // @[BusSlaveFactory.scala 942:69]
      end
      8'h50 : begin
        io_apb_PRDATA[1 : 0] = timerBBridge_ticksEnable; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[16 : 16] = timerBBridge_clearsEnable; // @[BusSlaveFactory.scala 942:69]
      end
      8'h54 : begin
        io_apb_PRDATA[15 : 0] = system_timer_timerB_io_limit_driver; // @[BusSlaveFactory.scala 942:69]
      end
      8'h58 : begin
        io_apb_PRDATA[15 : 0] = timerB_io_value; // @[BusSlaveFactory.scala 942:69]
      end
      8'h10 : begin
        io_apb_PRDATA[1 : 0] = interruptCtrl_1_io_pendings; // @[BusSlaveFactory.scala 942:69]
      end
      8'h14 : begin
        io_apb_PRDATA[1 : 0] = system_timer_interruptCtrl_1_io_masks_driver; // @[BusSlaveFactory.scala 942:69]
      end
      default : begin
      end
    endcase
  end

  assign busCtrl_askWrite = ((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PWRITE); // @[BaseType.scala 305:24]
  assign busCtrl_askRead = ((io_apb_PSEL[0] && io_apb_PENABLE) && (! io_apb_PWRITE)); // @[BaseType.scala 305:24]
  assign busCtrl_doWrite = (((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PREADY) && io_apb_PWRITE); // @[BaseType.scala 305:24]
  assign busCtrl_doRead = (((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PREADY) && (! io_apb_PWRITE)); // @[BaseType.scala 305:24]
  assign io_apb_PSLVERROR = ((busCtrl_doWrite && busCtrl_writeErrorFlag) || (busCtrl_doRead && busCtrl_readErrorFlag)); // @[Apb3SlaveFactory.scala 46:47]
  always @(*) begin
    _zz_io_clear = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      8'h0 : begin
        if(busCtrl_doWrite) begin
          _zz_io_clear = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    timerABridge_busClearing = 1'b0; // @[Timer.scala 36:24]
    if(when_Timer_l40) begin
      timerABridge_busClearing = 1'b1; // @[Timer.scala 40:24]
    end
    if(when_Timer_l44) begin
      timerABridge_busClearing = 1'b1; // @[Timer.scala 44:24]
    end
  end

  always @(*) begin
    when_Timer_l40 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      8'h44 : begin
        if(busCtrl_doWrite) begin
          when_Timer_l40 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    when_Timer_l44 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      8'h48 : begin
        if(busCtrl_doWrite) begin
          when_Timer_l44 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign timerA_io_clear = ((|(timerABridge_clearsEnable & timerA_io_full)) || timerABridge_busClearing); // @[Timer.scala 46:14]
  assign timerA_io_tick = (|(timerABridge_ticksEnable & {prescaler_1_io_overflow,1'b1})); // @[Timer.scala 47:14]
  always @(*) begin
    timerBBridge_busClearing = 1'b0; // @[Timer.scala 36:24]
    if(when_Timer_l40_1) begin
      timerBBridge_busClearing = 1'b1; // @[Timer.scala 40:24]
    end
    if(when_Timer_l44_1) begin
      timerBBridge_busClearing = 1'b1; // @[Timer.scala 44:24]
    end
  end

  always @(*) begin
    when_Timer_l40_1 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      8'h54 : begin
        if(busCtrl_doWrite) begin
          when_Timer_l40_1 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    when_Timer_l44_1 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      8'h58 : begin
        if(busCtrl_doWrite) begin
          when_Timer_l44_1 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign timerB_io_clear = ((|(timerBBridge_clearsEnable & timerB_io_full)) || timerBBridge_busClearing); // @[Timer.scala 46:14]
  assign timerB_io_tick = (|(timerBBridge_ticksEnable & {prescaler_1_io_overflow,1'b1})); // @[Timer.scala 47:14]
  always @(*) begin
    interruptCtrl_1_io_clears = 2'b00; // @[InterruptCtrl.scala 21:15]
    case(io_apb_PADDR)
      8'h10 : begin
        if(busCtrl_doWrite) begin
          interruptCtrl_1_io_clears = io_apb_PWDATA[1 : 0]; // @[Bits.scala 133:56]
        end
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    interruptCtrl_1_io_inputs[0] = timerA_io_full; // @[MuraxUtiles.scala 166:30]
    interruptCtrl_1_io_inputs[1] = timerB_io_full; // @[MuraxUtiles.scala 167:30]
  end

  assign io_interrupt = (|interruptCtrl_1_io_pendings); // @[MuraxUtiles.scala 168:16]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      timerABridge_ticksEnable <= 2'b00; // @[Data.scala 400:33]
      timerABridge_clearsEnable <= 1'b0; // @[Data.scala 400:33]
      timerBBridge_ticksEnable <= 2'b00; // @[Data.scala 400:33]
      timerBBridge_clearsEnable <= 1'b0; // @[Data.scala 400:33]
      system_timer_interruptCtrl_1_io_masks_driver <= 2'b00; // @[Data.scala 400:33]
    end else begin
      case(io_apb_PADDR)
        8'h40 : begin
          if(busCtrl_doWrite) begin
            timerABridge_ticksEnable <= io_apb_PWDATA[1 : 0]; // @[Bits.scala 133:56]
            timerABridge_clearsEnable <= io_apb_PWDATA[16 : 16]; // @[Bits.scala 133:56]
          end
        end
        8'h50 : begin
          if(busCtrl_doWrite) begin
            timerBBridge_ticksEnable <= io_apb_PWDATA[1 : 0]; // @[Bits.scala 133:56]
            timerBBridge_clearsEnable <= io_apb_PWDATA[16 : 16]; // @[Bits.scala 133:56]
          end
        end
        8'h14 : begin
          if(busCtrl_doWrite) begin
            system_timer_interruptCtrl_1_io_masks_driver <= io_apb_PWDATA[1 : 0]; // @[Bits.scala 133:56]
          end
        end
        default : begin
        end
      endcase
    end
  end

  always @(posedge io_mainClk) begin
    case(io_apb_PADDR)
      8'h0 : begin
        if(busCtrl_doWrite) begin
          _zz_io_limit <= io_apb_PWDATA[15 : 0]; // @[UInt.scala 381:56]
        end
      end
      8'h44 : begin
        if(busCtrl_doWrite) begin
          system_timer_timerA_io_limit_driver <= io_apb_PWDATA[15 : 0]; // @[UInt.scala 381:56]
        end
      end
      8'h54 : begin
        if(busCtrl_doWrite) begin
          system_timer_timerB_io_limit_driver <= io_apb_PWDATA[15 : 0]; // @[UInt.scala 381:56]
        end
      end
      default : begin
      end
    endcase
  end


endmodule

module Apb3UartCtrl (
  input      [4:0]    io_apb_PADDR,
  input      [0:0]    io_apb_PSEL,
  input               io_apb_PENABLE,
  output              io_apb_PREADY,
  input               io_apb_PWRITE,
  input      [31:0]   io_apb_PWDATA,
  output reg [31:0]   io_apb_PRDATA,
  output              io_uart_txd,
  input               io_uart_rxd,
  output              io_interrupt,
  input               io_mainClk,
  input               resetCtrl_systemReset
);
  localparam UartStopType_ONE = 1'd0;
  localparam UartStopType_TWO = 1'd1;
  localparam UartParityType_NONE = 2'd0;
  localparam UartParityType_EVEN = 2'd1;
  localparam UartParityType_ODD = 2'd2;

  reg                 system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_ready;
  wire                uartCtrl_1_io_write_ready;
  wire                uartCtrl_1_io_read_valid;
  wire       [7:0]    uartCtrl_1_io_read_payload;
  wire                uartCtrl_1_io_uart_txd;
  wire                uartCtrl_1_io_readError;
  wire                uartCtrl_1_io_readBreak;
  wire                bridge_write_streamUnbuffered_queueWithOccupancy_io_push_ready;
  wire                bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_valid;
  wire       [7:0]    bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_payload;
  wire       [4:0]    bridge_write_streamUnbuffered_queueWithOccupancy_io_occupancy;
  wire       [4:0]    bridge_write_streamUnbuffered_queueWithOccupancy_io_availability;
  wire                system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_push_ready;
  wire                system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_valid;
  wire       [7:0]    system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_payload;
  wire       [4:0]    system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_occupancy;
  wire       [4:0]    system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_availability;
  wire       [0:0]    _zz_bridge_misc_readError;
  wire       [0:0]    _zz_bridge_misc_readOverflowError;
  wire       [0:0]    _zz_bridge_misc_breakDetected;
  wire       [0:0]    _zz_bridge_misc_doBreak;
  wire       [0:0]    _zz_bridge_misc_doBreak_1;
  wire       [4:0]    _zz_io_apb_PRDATA;
  wire                busCtrl_readErrorFlag;
  wire                busCtrl_writeErrorFlag;
  wire                busCtrl_askWrite;
  wire                busCtrl_askRead;
  wire                busCtrl_doWrite;
  wire                busCtrl_doRead;
  wire                bridge_busCtrlWrapped_readErrorFlag;
  wire                bridge_busCtrlWrapped_writeErrorFlag;
  wire       [2:0]    bridge_uartConfigReg_frame_dataLength;
  wire       [0:0]    bridge_uartConfigReg_frame_stop;
  wire       [1:0]    bridge_uartConfigReg_frame_parity;
  reg        [19:0]   bridge_uartConfigReg_clockDivider;
  reg                 _zz_bridge_write_streamUnbuffered_valid;
  wire                bridge_write_streamUnbuffered_valid;
  wire                bridge_write_streamUnbuffered_ready;
  wire       [7:0]    bridge_write_streamUnbuffered_payload;
  reg                 bridge_read_streamBreaked_valid;
  reg                 bridge_read_streamBreaked_ready;
  wire       [7:0]    bridge_read_streamBreaked_payload;
  reg                 bridge_interruptCtrl_writeIntEnable;
  reg                 bridge_interruptCtrl_readIntEnable;
  wire                bridge_interruptCtrl_readInt;
  wire                bridge_interruptCtrl_writeInt;
  wire                bridge_interruptCtrl_interrupt;
  reg                 bridge_misc_readError;
  reg                 when_BusSlaveFactory_l341;
  wire                when_BusSlaveFactory_l347;
  reg                 bridge_misc_readOverflowError;
  reg                 when_BusSlaveFactory_l341_1;
  wire                when_BusSlaveFactory_l347_1;
  wire                system_uartCtrl_uartCtrl_1_io_read_isStall;
  reg                 bridge_misc_breakDetected;
  reg                 system_uartCtrl_uartCtrl_1_io_readBreak_regNext;
  wire                when_UartCtrl_l155;
  reg                 when_BusSlaveFactory_l341_2;
  wire                when_BusSlaveFactory_l347_2;
  reg                 bridge_misc_doBreak;
  reg                 when_BusSlaveFactory_l377;
  wire                when_BusSlaveFactory_l379;
  reg                 when_BusSlaveFactory_l341_3;
  wire                when_BusSlaveFactory_l347_3;
  `ifndef SYNTHESIS
  reg [23:0] bridge_uartConfigReg_frame_stop_string;
  reg [31:0] bridge_uartConfigReg_frame_parity_string;
  `endif

  function [19:0] zz_bridge_uartConfigReg_clockDivider(input dummy);
    begin
      zz_bridge_uartConfigReg_clockDivider = 20'h0;
      zz_bridge_uartConfigReg_clockDivider = 20'h00013;
    end
  endfunction
  wire [19:0] _zz_1;

  assign _zz_bridge_misc_readError = 1'b0;
  assign _zz_bridge_misc_readOverflowError = 1'b0;
  assign _zz_bridge_misc_breakDetected = 1'b0;
  assign _zz_bridge_misc_doBreak = 1'b1;
  assign _zz_bridge_misc_doBreak_1 = 1'b0;
  assign _zz_io_apb_PRDATA = (5'h10 - bridge_write_streamUnbuffered_queueWithOccupancy_io_occupancy);
  UartCtrl uartCtrl_1 (
    .io_config_frame_dataLength (bridge_uartConfigReg_frame_dataLength[2:0]                          ), //i
    .io_config_frame_stop       (bridge_uartConfigReg_frame_stop                                     ), //i
    .io_config_frame_parity     (bridge_uartConfigReg_frame_parity[1:0]                              ), //i
    .io_config_clockDivider     (bridge_uartConfigReg_clockDivider[19:0]                             ), //i
    .io_write_valid             (bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_valid       ), //i
    .io_write_ready             (uartCtrl_1_io_write_ready                                           ), //o
    .io_write_payload           (bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_payload[7:0]), //i
    .io_read_valid              (uartCtrl_1_io_read_valid                                            ), //o
    .io_read_ready              (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_push_ready ), //i
    .io_read_payload            (uartCtrl_1_io_read_payload[7:0]                                     ), //o
    .io_uart_txd                (uartCtrl_1_io_uart_txd                                              ), //o
    .io_uart_rxd                (io_uart_rxd                                                         ), //i
    .io_readError               (uartCtrl_1_io_readError                                             ), //o
    .io_writeBreak              (bridge_misc_doBreak                                                 ), //i
    .io_readBreak               (uartCtrl_1_io_readBreak                                             ), //o
    .io_mainClk                 (io_mainClk                                                          ), //i
    .resetCtrl_systemReset      (resetCtrl_systemReset                                               )  //i
  );
  StreamFifo_1 bridge_write_streamUnbuffered_queueWithOccupancy (
    .io_push_valid         (bridge_write_streamUnbuffered_valid                                  ), //i
    .io_push_ready         (bridge_write_streamUnbuffered_queueWithOccupancy_io_push_ready       ), //o
    .io_push_payload       (bridge_write_streamUnbuffered_payload[7:0]                           ), //i
    .io_pop_valid          (bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_valid        ), //o
    .io_pop_ready          (uartCtrl_1_io_write_ready                                            ), //i
    .io_pop_payload        (bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_payload[7:0] ), //o
    .io_flush              (1'b0                                                                 ), //i
    .io_occupancy          (bridge_write_streamUnbuffered_queueWithOccupancy_io_occupancy[4:0]   ), //o
    .io_availability       (bridge_write_streamUnbuffered_queueWithOccupancy_io_availability[4:0]), //o
    .io_mainClk            (io_mainClk                                                           ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                                                )  //i
  );
  StreamFifo_1 system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy (
    .io_push_valid         (uartCtrl_1_io_read_valid                                                  ), //i
    .io_push_ready         (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_push_ready       ), //o
    .io_push_payload       (uartCtrl_1_io_read_payload[7:0]                                           ), //i
    .io_pop_valid          (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_valid        ), //o
    .io_pop_ready          (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_ready        ), //i
    .io_pop_payload        (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_payload[7:0] ), //o
    .io_flush              (1'b0                                                                      ), //i
    .io_occupancy          (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_occupancy[4:0]   ), //o
    .io_availability       (system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_availability[4:0]), //o
    .io_mainClk            (io_mainClk                                                                ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                                                     )  //i
  );
  `ifndef SYNTHESIS
  always @(*) begin
    case(bridge_uartConfigReg_frame_stop)
      UartStopType_ONE : bridge_uartConfigReg_frame_stop_string = "ONE";
      UartStopType_TWO : bridge_uartConfigReg_frame_stop_string = "TWO";
      default : bridge_uartConfigReg_frame_stop_string = "???";
    endcase
  end
  always @(*) begin
    case(bridge_uartConfigReg_frame_parity)
      UartParityType_NONE : bridge_uartConfigReg_frame_parity_string = "NONE";
      UartParityType_EVEN : bridge_uartConfigReg_frame_parity_string = "EVEN";
      UartParityType_ODD : bridge_uartConfigReg_frame_parity_string = "ODD ";
      default : bridge_uartConfigReg_frame_parity_string = "????";
    endcase
  end
  `endif

  assign io_uart_txd = uartCtrl_1_io_uart_txd; // @[Apb3UartCtrl.scala 26:11]
  assign busCtrl_readErrorFlag = 1'b0; // @[BusSlaveFactory.scala 105:29]
  assign busCtrl_writeErrorFlag = 1'b0; // @[BusSlaveFactory.scala 106:30]
  assign io_apb_PREADY = 1'b1; // @[Apb3SlaveFactory.scala 38:14]
  always @(*) begin
    io_apb_PRDATA = 32'h0; // @[Apb3SlaveFactory.scala 39:68]
    case(io_apb_PADDR)
      5'h0 : begin
        io_apb_PRDATA[16 : 16] = (bridge_read_streamBreaked_valid ^ 1'b0); // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[7 : 0] = bridge_read_streamBreaked_payload; // @[BusSlaveFactory.scala 942:69]
      end
      5'h04 : begin
        io_apb_PRDATA[20 : 16] = _zz_io_apb_PRDATA; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[15 : 15] = bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_valid; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[28 : 24] = system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_occupancy; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[0 : 0] = bridge_interruptCtrl_writeIntEnable; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[1 : 1] = bridge_interruptCtrl_readIntEnable; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[8 : 8] = bridge_interruptCtrl_writeInt; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[9 : 9] = bridge_interruptCtrl_readInt; // @[BusSlaveFactory.scala 942:69]
      end
      5'h10 : begin
        io_apb_PRDATA[0 : 0] = bridge_misc_readError; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[1 : 1] = bridge_misc_readOverflowError; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[8 : 8] = uartCtrl_1_io_readBreak; // @[BusSlaveFactory.scala 942:69]
        io_apb_PRDATA[9 : 9] = bridge_misc_breakDetected; // @[BusSlaveFactory.scala 942:69]
      end
      default : begin
      end
    endcase
  end

  assign busCtrl_askWrite = ((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PWRITE); // @[BaseType.scala 305:24]
  assign busCtrl_askRead = ((io_apb_PSEL[0] && io_apb_PENABLE) && (! io_apb_PWRITE)); // @[BaseType.scala 305:24]
  assign busCtrl_doWrite = (((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PREADY) && io_apb_PWRITE); // @[BaseType.scala 305:24]
  assign busCtrl_doRead = (((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PREADY) && (! io_apb_PWRITE)); // @[BaseType.scala 305:24]
  assign bridge_busCtrlWrapped_readErrorFlag = 1'b0; // @[BusSlaveFactory.scala 105:29]
  assign bridge_busCtrlWrapped_writeErrorFlag = 1'b0; // @[BusSlaveFactory.scala 106:30]
  assign _zz_1 = zz_bridge_uartConfigReg_clockDivider(1'b0);
  always @(*) bridge_uartConfigReg_clockDivider = _zz_1;
  assign bridge_uartConfigReg_frame_dataLength = 3'b111;
  assign bridge_uartConfigReg_frame_parity = UartParityType_NONE;
  assign bridge_uartConfigReg_frame_stop = UartStopType_ONE;
  always @(*) begin
    _zz_bridge_write_streamUnbuffered_valid = 1'b0; // @[BusSlaveFactory.scala 513:18]
    case(io_apb_PADDR)
      5'h0 : begin
        if(busCtrl_doWrite) begin
          _zz_bridge_write_streamUnbuffered_valid = 1'b1; // @[BusSlaveFactory.scala 514:36]
        end
      end
      default : begin
      end
    endcase
  end

  assign bridge_write_streamUnbuffered_valid = _zz_bridge_write_streamUnbuffered_valid; // @[Flow.scala 72:15]
  assign bridge_write_streamUnbuffered_payload = io_apb_PWDATA[7 : 0]; // @[Flow.scala 73:17]
  assign bridge_write_streamUnbuffered_ready = bridge_write_streamUnbuffered_queueWithOccupancy_io_push_ready; // @[Stream.scala 295:16]
  always @(*) begin
    bridge_read_streamBreaked_valid = system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_valid; // @[Stream.scala 294:16]
    if(uartCtrl_1_io_readBreak) begin
      bridge_read_streamBreaked_valid = 1'b0; // @[Stream.scala 439:18]
    end
  end

  always @(*) begin
    system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_ready = bridge_read_streamBreaked_ready; // @[Stream.scala 295:16]
    if(uartCtrl_1_io_readBreak) begin
      system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_ready = 1'b1; // @[Stream.scala 440:18]
    end
  end

  assign bridge_read_streamBreaked_payload = system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_pop_payload; // @[Stream.scala 296:18]
  always @(*) begin
    bridge_read_streamBreaked_ready = 1'b0; // @[BusSlaveFactory.scala 576:16]
    case(io_apb_PADDR)
      5'h0 : begin
        if(busCtrl_doRead) begin
          bridge_read_streamBreaked_ready = 1'b1; // @[BusSlaveFactory.scala 578:18]
        end
      end
      default : begin
      end
    endcase
  end

  assign bridge_interruptCtrl_readInt = (bridge_interruptCtrl_readIntEnable && bridge_read_streamBreaked_valid); // @[BaseType.scala 305:24]
  assign bridge_interruptCtrl_writeInt = (bridge_interruptCtrl_writeIntEnable && (! bridge_write_streamUnbuffered_queueWithOccupancy_io_pop_valid)); // @[BaseType.scala 305:24]
  assign bridge_interruptCtrl_interrupt = (bridge_interruptCtrl_readInt || bridge_interruptCtrl_writeInt); // @[BaseType.scala 305:24]
  always @(*) begin
    when_BusSlaveFactory_l341 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      5'h10 : begin
        if(busCtrl_doWrite) begin
          when_BusSlaveFactory_l341 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign when_BusSlaveFactory_l347 = io_apb_PWDATA[0]; // @[BaseType.scala 305:24]
  always @(*) begin
    when_BusSlaveFactory_l341_1 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      5'h10 : begin
        if(busCtrl_doWrite) begin
          when_BusSlaveFactory_l341_1 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign when_BusSlaveFactory_l347_1 = io_apb_PWDATA[1]; // @[BaseType.scala 305:24]
  assign system_uartCtrl_uartCtrl_1_io_read_isStall = (uartCtrl_1_io_read_valid && (! system_uartCtrl_uartCtrl_1_io_read_queueWithOccupancy_io_push_ready)); // @[BaseType.scala 305:24]
  assign when_UartCtrl_l155 = (uartCtrl_1_io_readBreak && (! system_uartCtrl_uartCtrl_1_io_readBreak_regNext)); // @[BaseType.scala 305:24]
  always @(*) begin
    when_BusSlaveFactory_l341_2 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      5'h10 : begin
        if(busCtrl_doWrite) begin
          when_BusSlaveFactory_l341_2 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign when_BusSlaveFactory_l347_2 = io_apb_PWDATA[9]; // @[BaseType.scala 305:24]
  always @(*) begin
    when_BusSlaveFactory_l377 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      5'h10 : begin
        if(busCtrl_doWrite) begin
          when_BusSlaveFactory_l377 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign when_BusSlaveFactory_l379 = io_apb_PWDATA[10]; // @[BaseType.scala 305:24]
  always @(*) begin
    when_BusSlaveFactory_l341_3 = 1'b0; // @[BusSlaveFactory.scala 204:15]
    case(io_apb_PADDR)
      5'h10 : begin
        if(busCtrl_doWrite) begin
          when_BusSlaveFactory_l341_3 = 1'b1; // @[BusSlaveFactory.scala 205:27]
        end
      end
      default : begin
      end
    endcase
  end

  assign when_BusSlaveFactory_l347_3 = io_apb_PWDATA[11]; // @[BaseType.scala 305:24]
  assign io_interrupt = bridge_interruptCtrl_interrupt; // @[Apb3UartCtrl.scala 30:16]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      bridge_interruptCtrl_writeIntEnable <= 1'b0; // @[Data.scala 400:33]
      bridge_interruptCtrl_readIntEnable <= 1'b0; // @[Data.scala 400:33]
      bridge_misc_readError <= 1'b0; // @[Data.scala 400:33]
      bridge_misc_readOverflowError <= 1'b0; // @[Data.scala 400:33]
      bridge_misc_breakDetected <= 1'b0; // @[Data.scala 400:33]
      bridge_misc_doBreak <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(when_BusSlaveFactory_l341) begin
        if(when_BusSlaveFactory_l347) begin
          bridge_misc_readError <= _zz_bridge_misc_readError[0]; // @[Bool.scala 189:10]
        end
      end
      if(uartCtrl_1_io_readError) begin
        bridge_misc_readError <= 1'b1; // @[UartCtrl.scala 152:91]
      end
      if(when_BusSlaveFactory_l341_1) begin
        if(when_BusSlaveFactory_l347_1) begin
          bridge_misc_readOverflowError <= _zz_bridge_misc_readOverflowError[0]; // @[Bool.scala 189:10]
        end
      end
      if(system_uartCtrl_uartCtrl_1_io_read_isStall) begin
        bridge_misc_readOverflowError <= 1'b1; // @[UartCtrl.scala 153:99]
      end
      if(when_UartCtrl_l155) begin
        bridge_misc_breakDetected <= 1'b1; // @[UartCtrl.scala 155:42]
      end
      if(when_BusSlaveFactory_l341_2) begin
        if(when_BusSlaveFactory_l347_2) begin
          bridge_misc_breakDetected <= _zz_bridge_misc_breakDetected[0]; // @[Bool.scala 189:10]
        end
      end
      if(when_BusSlaveFactory_l377) begin
        if(when_BusSlaveFactory_l379) begin
          bridge_misc_doBreak <= _zz_bridge_misc_doBreak[0]; // @[Bool.scala 189:10]
        end
      end
      if(when_BusSlaveFactory_l341_3) begin
        if(when_BusSlaveFactory_l347_3) begin
          bridge_misc_doBreak <= _zz_bridge_misc_doBreak_1[0]; // @[Bool.scala 189:10]
        end
      end
      case(io_apb_PADDR)
        5'h04 : begin
          if(busCtrl_doWrite) begin
            bridge_interruptCtrl_writeIntEnable <= io_apb_PWDATA[0]; // @[Bool.scala 189:10]
            bridge_interruptCtrl_readIntEnable <= io_apb_PWDATA[1]; // @[Bool.scala 189:10]
          end
        end
        default : begin
        end
      endcase
    end
  end

  always @(posedge io_mainClk) begin
    system_uartCtrl_uartCtrl_1_io_readBreak_regNext <= uartCtrl_1_io_readBreak; // @[Reg.scala 39:30]
  end


endmodule

module Apb3Gpio (
  input      [3:0]    io_apb_PADDR,
  input      [0:0]    io_apb_PSEL,
  input               io_apb_PENABLE,
  output              io_apb_PREADY,
  input               io_apb_PWRITE,
  input      [31:0]   io_apb_PWDATA,
  output reg [31:0]   io_apb_PRDATA,
  output              io_apb_PSLVERROR,
  input      [31:0]   io_gpio_read,
  output     [31:0]   io_gpio_write,
  output     [31:0]   io_gpio_writeEnable,
  output     [31:0]   io_value,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  wire       [31:0]   io_gpio_read_buffercc_io_dataOut;
  wire                ctrl_readErrorFlag;
  wire                ctrl_writeErrorFlag;
  wire                ctrl_askWrite;
  wire                ctrl_askRead;
  wire                ctrl_doWrite;
  wire                ctrl_doRead;
  reg        [31:0]   io_gpio_write_driver;
  reg        [31:0]   io_gpio_writeEnable_driver;

  BufferCC_1 io_gpio_read_buffercc (
    .io_dataIn             (io_gpio_read[31:0]                    ), //i
    .io_dataOut            (io_gpio_read_buffercc_io_dataOut[31:0]), //o
    .io_mainClk            (io_mainClk                            ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                 )  //i
  );
  assign io_value = io_gpio_read_buffercc_io_dataOut; // @[Apb3Gpio.scala 53:12]
  assign ctrl_readErrorFlag = 1'b0; // @[BusSlaveFactory.scala 105:29]
  assign ctrl_writeErrorFlag = 1'b0; // @[BusSlaveFactory.scala 106:30]
  assign io_apb_PREADY = 1'b1; // @[Apb3SlaveFactory.scala 38:14]
  always @(*) begin
    io_apb_PRDATA = 32'h0; // @[Apb3SlaveFactory.scala 39:68]
    case(io_apb_PADDR)
      4'b0000 : begin
        io_apb_PRDATA[31 : 0] = io_value; // @[BusSlaveFactory.scala 942:69]
      end
      4'b0100 : begin
        io_apb_PRDATA[31 : 0] = io_gpio_write_driver; // @[BusSlaveFactory.scala 942:69]
      end
      4'b1000 : begin
        io_apb_PRDATA[31 : 0] = io_gpio_writeEnable_driver; // @[BusSlaveFactory.scala 942:69]
      end
      default : begin
      end
    endcase
  end

  assign ctrl_askWrite = ((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PWRITE); // @[BaseType.scala 305:24]
  assign ctrl_askRead = ((io_apb_PSEL[0] && io_apb_PENABLE) && (! io_apb_PWRITE)); // @[BaseType.scala 305:24]
  assign ctrl_doWrite = (((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PREADY) && io_apb_PWRITE); // @[BaseType.scala 305:24]
  assign ctrl_doRead = (((io_apb_PSEL[0] && io_apb_PENABLE) && io_apb_PREADY) && (! io_apb_PWRITE)); // @[BaseType.scala 305:24]
  assign io_apb_PSLVERROR = ((ctrl_doWrite && ctrl_writeErrorFlag) || (ctrl_doRead && ctrl_readErrorFlag)); // @[Apb3SlaveFactory.scala 46:47]
  assign io_gpio_write = io_gpio_write_driver; // @[BusSlaveFactory.scala 476:10]
  assign io_gpio_writeEnable = io_gpio_writeEnable_driver; // @[BusSlaveFactory.scala 476:10]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      io_gpio_writeEnable_driver <= 32'h0; // @[Data.scala 400:33]
    end else begin
      case(io_apb_PADDR)
        4'b1000 : begin
          if(ctrl_doWrite) begin
            io_gpio_writeEnable_driver <= io_apb_PWDATA[31 : 0]; // @[Bits.scala 133:56]
          end
        end
        default : begin
        end
      endcase
    end
  end

  always @(posedge io_mainClk) begin
    case(io_apb_PADDR)
      4'b0100 : begin
        if(ctrl_doWrite) begin
          io_gpio_write_driver <= io_apb_PWDATA[31 : 0]; // @[Bits.scala 133:56]
        end
      end
      default : begin
      end
    endcase
  end


endmodule

module PipelinedMemoryBusToApbBridge (
  input               io_pipelinedMemoryBus_cmd_valid,
  output              io_pipelinedMemoryBus_cmd_ready,
  input               io_pipelinedMemoryBus_cmd_payload_write,
  input      [31:0]   io_pipelinedMemoryBus_cmd_payload_address,
  input      [31:0]   io_pipelinedMemoryBus_cmd_payload_data,
  input      [3:0]    io_pipelinedMemoryBus_cmd_payload_mask,
  output              io_pipelinedMemoryBus_rsp_valid,
  output     [31:0]   io_pipelinedMemoryBus_rsp_payload_data,
  output     [19:0]   io_apb_PADDR,
  output     [0:0]    io_apb_PSEL,
  output              io_apb_PENABLE,
  input               io_apb_PREADY,
  output              io_apb_PWRITE,
  output     [31:0]   io_apb_PWDATA,
  input      [31:0]   io_apb_PRDATA,
  input               io_apb_PSLVERROR,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  wire                pipelinedMemoryBusStage_cmd_valid;
  reg                 pipelinedMemoryBusStage_cmd_ready;
  wire                pipelinedMemoryBusStage_cmd_payload_write;
  wire       [31:0]   pipelinedMemoryBusStage_cmd_payload_address;
  wire       [31:0]   pipelinedMemoryBusStage_cmd_payload_data;
  wire       [3:0]    pipelinedMemoryBusStage_cmd_payload_mask;
  reg                 pipelinedMemoryBusStage_rsp_valid;
  wire       [31:0]   pipelinedMemoryBusStage_rsp_payload_data;
  wire                io_pipelinedMemoryBus_cmd_halfPipe_valid;
  wire                io_pipelinedMemoryBus_cmd_halfPipe_ready;
  wire                io_pipelinedMemoryBus_cmd_halfPipe_payload_write;
  wire       [31:0]   io_pipelinedMemoryBus_cmd_halfPipe_payload_address;
  wire       [31:0]   io_pipelinedMemoryBus_cmd_halfPipe_payload_data;
  wire       [3:0]    io_pipelinedMemoryBus_cmd_halfPipe_payload_mask;
  reg                 io_pipelinedMemoryBus_cmd_rValid;
  wire                io_pipelinedMemoryBus_cmd_halfPipe_fire;
  reg                 io_pipelinedMemoryBus_cmd_rData_write;
  reg        [31:0]   io_pipelinedMemoryBus_cmd_rData_address;
  reg        [31:0]   io_pipelinedMemoryBus_cmd_rData_data;
  reg        [3:0]    io_pipelinedMemoryBus_cmd_rData_mask;
  reg                 pipelinedMemoryBusStage_rsp_regNext_valid;
  reg        [31:0]   pipelinedMemoryBusStage_rsp_regNext_payload_data;
  reg                 state;
  wire                when_PipelinedMemoryBus_l369;

  assign io_pipelinedMemoryBus_cmd_halfPipe_fire = (io_pipelinedMemoryBus_cmd_halfPipe_valid && io_pipelinedMemoryBus_cmd_halfPipe_ready); // @[BaseType.scala 305:24]
  assign io_pipelinedMemoryBus_cmd_ready = (! io_pipelinedMemoryBus_cmd_rValid); // @[Stream.scala 414:16]
  assign io_pipelinedMemoryBus_cmd_halfPipe_valid = io_pipelinedMemoryBus_cmd_rValid; // @[Stream.scala 416:20]
  assign io_pipelinedMemoryBus_cmd_halfPipe_payload_write = io_pipelinedMemoryBus_cmd_rData_write; // @[Stream.scala 417:22]
  assign io_pipelinedMemoryBus_cmd_halfPipe_payload_address = io_pipelinedMemoryBus_cmd_rData_address; // @[Stream.scala 417:22]
  assign io_pipelinedMemoryBus_cmd_halfPipe_payload_data = io_pipelinedMemoryBus_cmd_rData_data; // @[Stream.scala 417:22]
  assign io_pipelinedMemoryBus_cmd_halfPipe_payload_mask = io_pipelinedMemoryBus_cmd_rData_mask; // @[Stream.scala 417:22]
  assign pipelinedMemoryBusStage_cmd_valid = io_pipelinedMemoryBus_cmd_halfPipe_valid; // @[Stream.scala 294:16]
  assign io_pipelinedMemoryBus_cmd_halfPipe_ready = pipelinedMemoryBusStage_cmd_ready; // @[Stream.scala 295:16]
  assign pipelinedMemoryBusStage_cmd_payload_write = io_pipelinedMemoryBus_cmd_halfPipe_payload_write; // @[Stream.scala 296:18]
  assign pipelinedMemoryBusStage_cmd_payload_address = io_pipelinedMemoryBus_cmd_halfPipe_payload_address; // @[Stream.scala 296:18]
  assign pipelinedMemoryBusStage_cmd_payload_data = io_pipelinedMemoryBus_cmd_halfPipe_payload_data; // @[Stream.scala 296:18]
  assign pipelinedMemoryBusStage_cmd_payload_mask = io_pipelinedMemoryBus_cmd_halfPipe_payload_mask; // @[Stream.scala 296:18]
  assign io_pipelinedMemoryBus_rsp_valid = pipelinedMemoryBusStage_rsp_regNext_valid; // @[Flow.scala 94:11]
  assign io_pipelinedMemoryBus_rsp_payload_data = pipelinedMemoryBusStage_rsp_regNext_payload_data; // @[Flow.scala 95:13]
  always @(*) begin
    pipelinedMemoryBusStage_cmd_ready = 1'b0; // @[PipelinedMemoryBus.scala 359:37]
    if(!when_PipelinedMemoryBus_l369) begin
      if(io_apb_PREADY) begin
        pipelinedMemoryBusStage_cmd_ready = 1'b1; // @[PipelinedMemoryBus.scala 375:41]
      end
    end
  end

  assign io_apb_PSEL[0] = pipelinedMemoryBusStage_cmd_valid; // @[PipelinedMemoryBus.scala 361:18]
  assign io_apb_PENABLE = state; // @[PipelinedMemoryBus.scala 362:18]
  assign io_apb_PWRITE = pipelinedMemoryBusStage_cmd_payload_write; // @[PipelinedMemoryBus.scala 363:18]
  assign io_apb_PADDR = pipelinedMemoryBusStage_cmd_payload_address[19:0]; // @[PipelinedMemoryBus.scala 364:18]
  assign io_apb_PWDATA = pipelinedMemoryBusStage_cmd_payload_data; // @[PipelinedMemoryBus.scala 365:18]
  always @(*) begin
    pipelinedMemoryBusStage_rsp_valid = 1'b0; // @[PipelinedMemoryBus.scala 367:37]
    if(!when_PipelinedMemoryBus_l369) begin
      if(io_apb_PREADY) begin
        pipelinedMemoryBusStage_rsp_valid = (! pipelinedMemoryBusStage_cmd_payload_write); // @[PipelinedMemoryBus.scala 374:41]
      end
    end
  end

  assign pipelinedMemoryBusStage_rsp_payload_data = io_apb_PRDATA; // @[PipelinedMemoryBus.scala 368:37]
  assign when_PipelinedMemoryBus_l369 = (! state); // @[BaseType.scala 299:24]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      io_pipelinedMemoryBus_cmd_rValid <= 1'b0; // @[Data.scala 400:33]
      pipelinedMemoryBusStage_rsp_regNext_valid <= 1'b0; // @[Data.scala 400:33]
      state <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(io_pipelinedMemoryBus_cmd_valid) begin
        io_pipelinedMemoryBus_cmd_rValid <= 1'b1; // @[Stream.scala 411:33]
      end
      if(io_pipelinedMemoryBus_cmd_halfPipe_fire) begin
        io_pipelinedMemoryBus_cmd_rValid <= 1'b0; // @[Stream.scala 411:53]
      end
      pipelinedMemoryBusStage_rsp_regNext_valid <= pipelinedMemoryBusStage_rsp_valid; // @[Reg.scala 39:30]
      if(when_PipelinedMemoryBus_l369) begin
        state <= pipelinedMemoryBusStage_cmd_valid; // @[PipelinedMemoryBus.scala 370:11]
      end else begin
        if(io_apb_PREADY) begin
          state <= 1'b0; // @[PipelinedMemoryBus.scala 373:13]
        end
      end
    end
  end

  always @(posedge io_mainClk) begin
    if(io_pipelinedMemoryBus_cmd_ready) begin
      io_pipelinedMemoryBus_cmd_rData_write <= io_pipelinedMemoryBus_cmd_payload_write; // @[Stream.scala 412:28]
      io_pipelinedMemoryBus_cmd_rData_address <= io_pipelinedMemoryBus_cmd_payload_address; // @[Stream.scala 412:28]
      io_pipelinedMemoryBus_cmd_rData_data <= io_pipelinedMemoryBus_cmd_payload_data; // @[Stream.scala 412:28]
      io_pipelinedMemoryBus_cmd_rData_mask <= io_pipelinedMemoryBus_cmd_payload_mask; // @[Stream.scala 412:28]
    end
    pipelinedMemoryBusStage_rsp_regNext_payload_data <= pipelinedMemoryBusStage_rsp_payload_data; // @[Reg.scala 39:30]
  end


endmodule

module MuraxPipelinedMemoryBusRam (
  input               io_bus_cmd_valid,
  output              io_bus_cmd_ready,
  input               io_bus_cmd_payload_write,
  input      [31:0]   io_bus_cmd_payload_address,
  input      [31:0]   io_bus_cmd_payload_data,
  input      [3:0]    io_bus_cmd_payload_mask,
  output              io_bus_rsp_valid,
  output     [31:0]   io_bus_rsp_payload_data,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg        [31:0]   _zz_ram_port0;
  wire       [15:0]   _zz_ram_port;
  wire       [15:0]   _zz_io_bus_rsp_payload_data_2;
  wire                io_bus_cmd_fire;
  reg                 _zz_io_bus_rsp_valid;
  wire       [29:0]   _zz_io_bus_rsp_payload_data;
  wire       [31:0]   _zz_io_bus_rsp_payload_data_1;
  reg [7:0] ram_symbol0 [0:65535];
  reg [7:0] ram_symbol1 [0:65535];
  reg [7:0] ram_symbol2 [0:65535];
  reg [7:0] ram_symbol3 [0:65535];
  reg [7:0] _zz_ramsymbol_read;
  reg [7:0] _zz_ramsymbol_read_1;
  reg [7:0] _zz_ramsymbol_read_2;
  reg [7:0] _zz_ramsymbol_read_3;

  assign _zz_io_bus_rsp_payload_data_2 = _zz_io_bus_rsp_payload_data[15:0];
  always @(*) begin
    _zz_ram_port0 = {_zz_ramsymbol_read_3, _zz_ramsymbol_read_2, _zz_ramsymbol_read_1, _zz_ramsymbol_read};
  end
  always @(posedge io_mainClk) begin
    if(io_bus_cmd_valid) begin
      _zz_ramsymbol_read <= ram_symbol0[_zz_io_bus_rsp_payload_data_2];
      _zz_ramsymbol_read_1 <= ram_symbol1[_zz_io_bus_rsp_payload_data_2];
      _zz_ramsymbol_read_2 <= ram_symbol2[_zz_io_bus_rsp_payload_data_2];
      _zz_ramsymbol_read_3 <= ram_symbol3[_zz_io_bus_rsp_payload_data_2];
    end
  end

  always @(posedge io_mainClk) begin
    if(io_bus_cmd_payload_mask[0] && io_bus_cmd_valid && io_bus_cmd_payload_write ) begin
      ram_symbol0[_zz_io_bus_rsp_payload_data_2] <= _zz_io_bus_rsp_payload_data_1[7 : 0];
    end
    if(io_bus_cmd_payload_mask[1] && io_bus_cmd_valid && io_bus_cmd_payload_write ) begin
      ram_symbol1[_zz_io_bus_rsp_payload_data_2] <= _zz_io_bus_rsp_payload_data_1[15 : 8];
    end
    if(io_bus_cmd_payload_mask[2] && io_bus_cmd_valid && io_bus_cmd_payload_write ) begin
      ram_symbol2[_zz_io_bus_rsp_payload_data_2] <= _zz_io_bus_rsp_payload_data_1[23 : 16];
    end
    if(io_bus_cmd_payload_mask[3] && io_bus_cmd_valid && io_bus_cmd_payload_write ) begin
      ram_symbol3[_zz_io_bus_rsp_payload_data_2] <= _zz_io_bus_rsp_payload_data_1[31 : 24];
    end
  end

  assign io_bus_cmd_fire = (io_bus_cmd_valid && io_bus_cmd_ready); // @[BaseType.scala 305:24]
  assign io_bus_rsp_valid = _zz_io_bus_rsp_valid; // @[MuraxUtiles.scala 58:20]
  assign _zz_io_bus_rsp_payload_data = (io_bus_cmd_payload_address >>> 2); // @[Data.scala 450:19]
  assign _zz_io_bus_rsp_payload_data_1 = io_bus_cmd_payload_data; // @[Bits.scala 152:9]
  assign io_bus_rsp_payload_data = _zz_ram_port0; // @[MuraxUtiles.scala 59:19]
  assign io_bus_cmd_ready = 1'b1; // @[MuraxUtiles.scala 66:20]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      _zz_io_bus_rsp_valid <= 1'b0; // @[Data.scala 400:33]
    end else begin
      _zz_io_bus_rsp_valid <= (io_bus_cmd_fire && (! io_bus_cmd_payload_write)); // @[Reg.scala 39:30]
    end
  end


endmodule

module SystemDebugger (
  input               io_remote_cmd_valid,
  output              io_remote_cmd_ready,
  input               io_remote_cmd_payload_last,
  input      [0:0]    io_remote_cmd_payload_fragment,
  output              io_remote_rsp_valid,
  input               io_remote_rsp_ready,
  output              io_remote_rsp_payload_error,
  output     [31:0]   io_remote_rsp_payload_data,
  output              io_mem_cmd_valid,
  input               io_mem_cmd_ready,
  output     [31:0]   io_mem_cmd_payload_address,
  output     [31:0]   io_mem_cmd_payload_data,
  output              io_mem_cmd_payload_wr,
  output     [1:0]    io_mem_cmd_payload_size,
  input               io_mem_rsp_valid,
  input      [31:0]   io_mem_rsp_payload,
  input               io_mainClk,
  input               resetCtrl_mainClkReset
);

  reg        [66:0]   dispatcher_dataShifter;
  reg                 dispatcher_dataLoaded;
  reg        [7:0]    dispatcher_headerShifter;
  wire       [7:0]    dispatcher_header;
  reg                 dispatcher_headerLoaded;
  reg        [2:0]    dispatcher_counter;
  wire                when_Fragment_l346;
  wire                when_Fragment_l349;
  wire       [66:0]   _zz_io_mem_cmd_payload_address;
  wire                io_mem_cmd_isStall;
  wire                when_Fragment_l372;

  assign dispatcher_header = dispatcher_headerShifter[7 : 0]; // @[BaseType.scala 299:24]
  assign when_Fragment_l346 = (dispatcher_headerLoaded == 1'b0); // @[BaseType.scala 305:24]
  assign when_Fragment_l349 = (dispatcher_counter == 3'b111); // @[BaseType.scala 305:24]
  assign io_remote_cmd_ready = (! dispatcher_dataLoaded); // @[Fragment.scala 363:15]
  assign _zz_io_mem_cmd_payload_address = dispatcher_dataShifter[66 : 0]; // @[BaseType.scala 299:24]
  assign io_mem_cmd_payload_address = _zz_io_mem_cmd_payload_address[31 : 0]; // @[UInt.scala 381:56]
  assign io_mem_cmd_payload_data = _zz_io_mem_cmd_payload_address[63 : 32]; // @[Bits.scala 133:56]
  assign io_mem_cmd_payload_wr = _zz_io_mem_cmd_payload_address[64]; // @[Bool.scala 189:10]
  assign io_mem_cmd_payload_size = _zz_io_mem_cmd_payload_address[66 : 65]; // @[UInt.scala 381:56]
  assign io_mem_cmd_valid = (dispatcher_dataLoaded && (dispatcher_header == 8'h0)); // @[Fragment.scala 369:16]
  assign io_mem_cmd_isStall = (io_mem_cmd_valid && (! io_mem_cmd_ready)); // @[BaseType.scala 305:24]
  assign when_Fragment_l372 = ((dispatcher_headerLoaded && dispatcher_dataLoaded) && (! io_mem_cmd_isStall)); // @[BaseType.scala 305:24]
  assign io_remote_rsp_valid = io_mem_rsp_valid; // @[SystemDebugger.scala 186:23]
  assign io_remote_rsp_payload_error = 1'b0; // @[SystemDebugger.scala 187:23]
  assign io_remote_rsp_payload_data = io_mem_rsp_payload; // @[SystemDebugger.scala 188:22]
  always @(posedge io_mainClk or posedge resetCtrl_mainClkReset) begin
    if(resetCtrl_mainClkReset) begin
      dispatcher_dataLoaded <= 1'b0; // @[Data.scala 400:33]
      dispatcher_headerLoaded <= 1'b0; // @[Data.scala 400:33]
      dispatcher_counter <= 3'b000; // @[Data.scala 400:33]
    end else begin
      if(io_remote_cmd_valid) begin
        if(when_Fragment_l346) begin
          dispatcher_counter <= (dispatcher_counter + 3'b001); // @[Fragment.scala 348:15]
          if(when_Fragment_l349) begin
            dispatcher_headerLoaded <= 1'b1; // @[Fragment.scala 350:22]
          end
        end
        if(io_remote_cmd_payload_last) begin
          dispatcher_headerLoaded <= 1'b1; // @[Fragment.scala 357:20]
          dispatcher_dataLoaded <= 1'b1; // @[Fragment.scala 358:18]
          dispatcher_counter <= 3'b000; // @[Fragment.scala 359:15]
        end
      end
      if(when_Fragment_l372) begin
        dispatcher_headerLoaded <= 1'b0; // @[Fragment.scala 373:18]
        dispatcher_dataLoaded <= 1'b0; // @[Fragment.scala 374:16]
      end
    end
  end

  always @(posedge io_mainClk) begin
    if(io_remote_cmd_valid) begin
      if(when_Fragment_l346) begin
        dispatcher_headerShifter <= ({io_remote_cmd_payload_fragment,dispatcher_headerShifter} >>> 1); // @[Fragment.scala 347:21]
      end else begin
        dispatcher_dataShifter <= ({io_remote_cmd_payload_fragment,dispatcher_dataShifter} >>> 1); // @[Fragment.scala 353:19]
      end
    end
  end


endmodule

module JtagBridge (
  input               io_jtag_tms,
  input               io_jtag_tdi,
  output              io_jtag_tdo,
  input               io_jtag_tck,
  output              io_remote_cmd_valid,
  input               io_remote_cmd_ready,
  output              io_remote_cmd_payload_last,
  output     [0:0]    io_remote_cmd_payload_fragment,
  input               io_remote_rsp_valid,
  output              io_remote_rsp_ready,
  input               io_remote_rsp_payload_error,
  input      [31:0]   io_remote_rsp_payload_data,
  input               io_mainClk,
  input               resetCtrl_mainClkReset
);
  localparam JtagState_RESET = 4'd0;
  localparam JtagState_IDLE = 4'd1;
  localparam JtagState_IR_SELECT = 4'd2;
  localparam JtagState_IR_CAPTURE = 4'd3;
  localparam JtagState_IR_SHIFT = 4'd4;
  localparam JtagState_IR_EXIT1 = 4'd5;
  localparam JtagState_IR_PAUSE = 4'd6;
  localparam JtagState_IR_EXIT2 = 4'd7;
  localparam JtagState_IR_UPDATE = 4'd8;
  localparam JtagState_DR_SELECT = 4'd9;
  localparam JtagState_DR_CAPTURE = 4'd10;
  localparam JtagState_DR_SHIFT = 4'd11;
  localparam JtagState_DR_EXIT1 = 4'd12;
  localparam JtagState_DR_PAUSE = 4'd13;
  localparam JtagState_DR_EXIT2 = 4'd14;
  localparam JtagState_DR_UPDATE = 4'd15;

  wire                flowCCByToggle_1_io_output_valid;
  wire                flowCCByToggle_1_io_output_payload_last;
  wire       [0:0]    flowCCByToggle_1_io_output_payload_fragment;
  wire       [3:0]    _zz_jtag_tap_isBypass;
  wire       [3:0]    _zz_jtag_tap_isBypass_1;
  wire       [1:0]    _zz_jtag_tap_instructionShift;
  wire                system_cmd_valid;
  wire                system_cmd_payload_last;
  wire       [0:0]    system_cmd_payload_fragment;
  wire                system_cmd_toStream_valid;
  wire                system_cmd_toStream_ready;
  wire                system_cmd_toStream_payload_last;
  wire       [0:0]    system_cmd_toStream_payload_fragment;
  (* async_reg = "true" *) reg                 system_rsp_valid;
  (* async_reg = "true" *) reg                 system_rsp_payload_error;
  (* async_reg = "true" *) reg        [31:0]   system_rsp_payload_data;
  wire                io_remote_rsp_fire;
  wire       [3:0]    jtag_tap_fsm_stateNext;
  reg        [3:0]    jtag_tap_fsm_state;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_1;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_2;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_3;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_4;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_5;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_6;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_7;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_8;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_9;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_10;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_11;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_12;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_13;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_14;
  wire       [3:0]    _zz_jtag_tap_fsm_stateNext_15;
  reg        [3:0]    _zz_jtag_tap_fsm_stateNext_16;
  reg        [3:0]    jtag_tap_instruction;
  reg        [3:0]    jtag_tap_instructionShift;
  reg                 jtag_tap_bypass;
  reg                 jtag_tap_tdoUnbufferd;
  reg                 jtag_tap_tdoDr;
  wire                jtag_tap_tdoIr;
  wire                jtag_tap_isBypass;
  reg                 jtag_tap_tdoUnbufferd_regNext;
  wire                jtag_idcodeArea_ctrl_tdi;
  wire                jtag_idcodeArea_ctrl_enable;
  wire                jtag_idcodeArea_ctrl_capture;
  wire                jtag_idcodeArea_ctrl_shift;
  wire                jtag_idcodeArea_ctrl_update;
  wire                jtag_idcodeArea_ctrl_reset;
  wire                jtag_idcodeArea_ctrl_tdo;
  reg        [31:0]   jtag_idcodeArea_shifter;
  wire                when_JtagTap_l120;
  wire                jtag_writeArea_ctrl_tdi;
  wire                jtag_writeArea_ctrl_enable;
  wire                jtag_writeArea_ctrl_capture;
  wire                jtag_writeArea_ctrl_shift;
  wire                jtag_writeArea_ctrl_update;
  wire                jtag_writeArea_ctrl_reset;
  wire                jtag_writeArea_ctrl_tdo;
  wire                jtag_writeArea_source_valid;
  wire                jtag_writeArea_source_payload_last;
  wire       [0:0]    jtag_writeArea_source_payload_fragment;
  reg                 jtag_writeArea_valid;
  reg                 jtag_writeArea_data;
  wire                jtag_readArea_ctrl_tdi;
  wire                jtag_readArea_ctrl_enable;
  wire                jtag_readArea_ctrl_capture;
  wire                jtag_readArea_ctrl_shift;
  wire                jtag_readArea_ctrl_update;
  wire                jtag_readArea_ctrl_reset;
  wire                jtag_readArea_ctrl_tdo;
  reg        [33:0]   jtag_readArea_full_shifter;
  `ifndef SYNTHESIS
  reg [79:0] jtag_tap_fsm_stateNext_string;
  reg [79:0] jtag_tap_fsm_state_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_1_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_2_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_3_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_4_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_5_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_6_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_7_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_8_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_9_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_10_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_11_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_12_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_13_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_14_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_15_string;
  reg [79:0] _zz_jtag_tap_fsm_stateNext_16_string;
  `endif


  assign _zz_jtag_tap_isBypass = jtag_tap_instruction;
  assign _zz_jtag_tap_isBypass_1 = 4'b1111;
  assign _zz_jtag_tap_instructionShift = 2'b01;
  FlowCCByToggle flowCCByToggle_1 (
    .io_input_valid             (jtag_writeArea_source_valid                ), //i
    .io_input_payload_last      (jtag_writeArea_source_payload_last         ), //i
    .io_input_payload_fragment  (jtag_writeArea_source_payload_fragment     ), //i
    .io_output_valid            (flowCCByToggle_1_io_output_valid           ), //o
    .io_output_payload_last     (flowCCByToggle_1_io_output_payload_last    ), //o
    .io_output_payload_fragment (flowCCByToggle_1_io_output_payload_fragment), //o
    .io_jtag_tck                (io_jtag_tck                                ), //i
    .io_mainClk                 (io_mainClk                                 ), //i
    .resetCtrl_mainClkReset     (resetCtrl_mainClkReset                     )  //i
  );
  initial begin
  `ifndef SYNTHESIS
    jtag_tap_fsm_state = {1{$urandom}};
  `endif
  end

  `ifndef SYNTHESIS
  always @(*) begin
    case(jtag_tap_fsm_stateNext)
      JtagState_RESET : jtag_tap_fsm_stateNext_string = "RESET     ";
      JtagState_IDLE : jtag_tap_fsm_stateNext_string = "IDLE      ";
      JtagState_IR_SELECT : jtag_tap_fsm_stateNext_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : jtag_tap_fsm_stateNext_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : jtag_tap_fsm_stateNext_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : jtag_tap_fsm_stateNext_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : jtag_tap_fsm_stateNext_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : jtag_tap_fsm_stateNext_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : jtag_tap_fsm_stateNext_string = "IR_UPDATE ";
      JtagState_DR_SELECT : jtag_tap_fsm_stateNext_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : jtag_tap_fsm_stateNext_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : jtag_tap_fsm_stateNext_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : jtag_tap_fsm_stateNext_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : jtag_tap_fsm_stateNext_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : jtag_tap_fsm_stateNext_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : jtag_tap_fsm_stateNext_string = "DR_UPDATE ";
      default : jtag_tap_fsm_stateNext_string = "??????????";
    endcase
  end
  always @(*) begin
    case(jtag_tap_fsm_state)
      JtagState_RESET : jtag_tap_fsm_state_string = "RESET     ";
      JtagState_IDLE : jtag_tap_fsm_state_string = "IDLE      ";
      JtagState_IR_SELECT : jtag_tap_fsm_state_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : jtag_tap_fsm_state_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : jtag_tap_fsm_state_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : jtag_tap_fsm_state_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : jtag_tap_fsm_state_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : jtag_tap_fsm_state_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : jtag_tap_fsm_state_string = "IR_UPDATE ";
      JtagState_DR_SELECT : jtag_tap_fsm_state_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : jtag_tap_fsm_state_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : jtag_tap_fsm_state_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : jtag_tap_fsm_state_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : jtag_tap_fsm_state_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : jtag_tap_fsm_state_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : jtag_tap_fsm_state_string = "DR_UPDATE ";
      default : jtag_tap_fsm_state_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_1)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_1_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_1_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_1_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_1_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_1_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_1_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_1_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_1_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_1_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_1_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_1_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_1_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_1_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_1_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_1_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_1_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_1_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_2)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_2_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_2_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_2_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_2_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_2_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_2_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_2_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_2_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_2_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_2_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_2_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_2_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_2_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_2_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_2_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_2_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_2_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_3)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_3_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_3_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_3_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_3_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_3_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_3_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_3_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_3_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_3_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_3_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_3_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_3_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_3_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_3_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_3_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_3_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_3_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_4)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_4_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_4_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_4_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_4_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_4_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_4_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_4_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_4_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_4_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_4_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_4_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_4_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_4_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_4_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_4_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_4_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_4_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_5)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_5_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_5_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_5_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_5_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_5_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_5_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_5_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_5_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_5_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_5_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_5_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_5_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_5_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_5_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_5_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_5_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_5_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_6)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_6_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_6_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_6_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_6_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_6_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_6_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_6_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_6_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_6_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_6_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_6_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_6_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_6_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_6_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_6_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_6_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_6_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_7)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_7_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_7_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_7_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_7_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_7_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_7_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_7_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_7_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_7_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_7_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_7_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_7_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_7_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_7_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_7_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_7_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_7_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_8)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_8_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_8_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_8_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_8_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_8_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_8_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_8_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_8_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_8_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_8_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_8_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_8_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_8_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_8_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_8_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_8_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_8_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_9)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_9_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_9_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_9_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_9_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_9_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_9_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_9_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_9_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_9_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_9_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_9_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_9_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_9_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_9_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_9_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_9_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_9_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_10)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_10_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_10_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_10_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_10_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_10_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_10_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_10_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_10_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_10_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_10_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_10_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_10_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_10_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_10_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_10_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_10_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_10_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_11)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_11_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_11_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_11_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_11_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_11_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_11_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_11_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_11_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_11_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_11_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_11_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_11_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_11_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_11_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_11_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_11_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_11_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_12)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_12_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_12_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_12_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_12_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_12_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_12_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_12_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_12_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_12_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_12_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_12_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_12_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_12_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_12_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_12_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_12_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_12_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_13)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_13_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_13_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_13_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_13_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_13_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_13_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_13_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_13_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_13_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_13_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_13_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_13_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_13_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_13_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_13_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_13_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_13_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_14)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_14_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_14_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_14_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_14_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_14_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_14_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_14_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_14_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_14_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_14_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_14_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_14_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_14_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_14_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_14_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_14_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_14_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_15)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_15_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_15_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_15_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_15_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_15_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_15_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_15_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_15_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_15_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_15_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_15_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_15_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_15_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_15_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_15_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_15_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_15_string = "??????????";
    endcase
  end
  always @(*) begin
    case(_zz_jtag_tap_fsm_stateNext_16)
      JtagState_RESET : _zz_jtag_tap_fsm_stateNext_16_string = "RESET     ";
      JtagState_IDLE : _zz_jtag_tap_fsm_stateNext_16_string = "IDLE      ";
      JtagState_IR_SELECT : _zz_jtag_tap_fsm_stateNext_16_string = "IR_SELECT ";
      JtagState_IR_CAPTURE : _zz_jtag_tap_fsm_stateNext_16_string = "IR_CAPTURE";
      JtagState_IR_SHIFT : _zz_jtag_tap_fsm_stateNext_16_string = "IR_SHIFT  ";
      JtagState_IR_EXIT1 : _zz_jtag_tap_fsm_stateNext_16_string = "IR_EXIT1  ";
      JtagState_IR_PAUSE : _zz_jtag_tap_fsm_stateNext_16_string = "IR_PAUSE  ";
      JtagState_IR_EXIT2 : _zz_jtag_tap_fsm_stateNext_16_string = "IR_EXIT2  ";
      JtagState_IR_UPDATE : _zz_jtag_tap_fsm_stateNext_16_string = "IR_UPDATE ";
      JtagState_DR_SELECT : _zz_jtag_tap_fsm_stateNext_16_string = "DR_SELECT ";
      JtagState_DR_CAPTURE : _zz_jtag_tap_fsm_stateNext_16_string = "DR_CAPTURE";
      JtagState_DR_SHIFT : _zz_jtag_tap_fsm_stateNext_16_string = "DR_SHIFT  ";
      JtagState_DR_EXIT1 : _zz_jtag_tap_fsm_stateNext_16_string = "DR_EXIT1  ";
      JtagState_DR_PAUSE : _zz_jtag_tap_fsm_stateNext_16_string = "DR_PAUSE  ";
      JtagState_DR_EXIT2 : _zz_jtag_tap_fsm_stateNext_16_string = "DR_EXIT2  ";
      JtagState_DR_UPDATE : _zz_jtag_tap_fsm_stateNext_16_string = "DR_UPDATE ";
      default : _zz_jtag_tap_fsm_stateNext_16_string = "??????????";
    endcase
  end
  `endif

  assign system_cmd_toStream_valid = system_cmd_valid; // @[Flow.scala 72:15]
  assign system_cmd_toStream_payload_last = system_cmd_payload_last; // @[Flow.scala 73:17]
  assign system_cmd_toStream_payload_fragment = system_cmd_payload_fragment; // @[Flow.scala 73:17]
  assign io_remote_cmd_valid = system_cmd_toStream_valid; // @[Stream.scala 294:16]
  assign system_cmd_toStream_ready = io_remote_cmd_ready; // @[Stream.scala 295:16]
  assign io_remote_cmd_payload_last = system_cmd_toStream_payload_last; // @[Stream.scala 296:18]
  assign io_remote_cmd_payload_fragment = system_cmd_toStream_payload_fragment; // @[Stream.scala 296:18]
  assign io_remote_rsp_fire = (io_remote_rsp_valid && io_remote_rsp_ready); // @[BaseType.scala 305:24]
  assign io_remote_rsp_ready = 1'b1; // @[SystemDebugger.scala 77:25]
  assign _zz_jtag_tap_fsm_stateNext = (io_jtag_tms ? JtagState_RESET : JtagState_IDLE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_1 = (io_jtag_tms ? JtagState_DR_SELECT : JtagState_IDLE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_2 = (io_jtag_tms ? JtagState_RESET : JtagState_IR_CAPTURE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_3 = (io_jtag_tms ? JtagState_IR_EXIT1 : JtagState_IR_SHIFT); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_4 = (io_jtag_tms ? JtagState_IR_EXIT1 : JtagState_IR_SHIFT); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_5 = (io_jtag_tms ? JtagState_IR_UPDATE : JtagState_IR_PAUSE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_6 = (io_jtag_tms ? JtagState_IR_EXIT2 : JtagState_IR_PAUSE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_7 = (io_jtag_tms ? JtagState_IR_UPDATE : JtagState_IR_SHIFT); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_8 = (io_jtag_tms ? JtagState_DR_SELECT : JtagState_IDLE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_9 = (io_jtag_tms ? JtagState_IR_SELECT : JtagState_DR_CAPTURE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_10 = (io_jtag_tms ? JtagState_DR_EXIT1 : JtagState_DR_SHIFT); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_11 = (io_jtag_tms ? JtagState_DR_EXIT1 : JtagState_DR_SHIFT); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_12 = (io_jtag_tms ? JtagState_DR_UPDATE : JtagState_DR_PAUSE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_13 = (io_jtag_tms ? JtagState_DR_EXIT2 : JtagState_DR_PAUSE); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_14 = (io_jtag_tms ? JtagState_DR_UPDATE : JtagState_DR_SHIFT); // @[Expression.scala 1420:25]
  assign _zz_jtag_tap_fsm_stateNext_15 = (io_jtag_tms ? JtagState_DR_SELECT : JtagState_IDLE); // @[Expression.scala 1420:25]
  always @(*) begin
    case(jtag_tap_fsm_state)
      JtagState_IDLE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_1; // @[Misc.scala 239:22]
      end
      JtagState_IR_SELECT : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_2; // @[Misc.scala 239:22]
      end
      JtagState_IR_CAPTURE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_3; // @[Misc.scala 239:22]
      end
      JtagState_IR_SHIFT : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_4; // @[Misc.scala 239:22]
      end
      JtagState_IR_EXIT1 : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_5; // @[Misc.scala 239:22]
      end
      JtagState_IR_PAUSE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_6; // @[Misc.scala 239:22]
      end
      JtagState_IR_EXIT2 : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_7; // @[Misc.scala 239:22]
      end
      JtagState_IR_UPDATE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_8; // @[Misc.scala 239:22]
      end
      JtagState_DR_SELECT : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_9; // @[Misc.scala 239:22]
      end
      JtagState_DR_CAPTURE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_10; // @[Misc.scala 239:22]
      end
      JtagState_DR_SHIFT : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_11; // @[Misc.scala 239:22]
      end
      JtagState_DR_EXIT1 : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_12; // @[Misc.scala 239:22]
      end
      JtagState_DR_PAUSE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_13; // @[Misc.scala 239:22]
      end
      JtagState_DR_EXIT2 : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_14; // @[Misc.scala 239:22]
      end
      JtagState_DR_UPDATE : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext_15; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_jtag_tap_fsm_stateNext_16 = _zz_jtag_tap_fsm_stateNext; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign jtag_tap_fsm_stateNext = _zz_jtag_tap_fsm_stateNext_16; // @[JtagTap.scala 50:13]
  always @(*) begin
    jtag_tap_tdoUnbufferd = jtag_tap_bypass; // @[Misc.scala 552:9]
    case(jtag_tap_fsm_state)
      JtagState_IR_SHIFT : begin
        jtag_tap_tdoUnbufferd = jtag_tap_tdoIr; // @[JtagTap.scala 91:20]
      end
      JtagState_DR_SHIFT : begin
        if(jtag_tap_isBypass) begin
          jtag_tap_tdoUnbufferd = jtag_tap_bypass; // @[JtagTap.scala 99:22]
        end else begin
          jtag_tap_tdoUnbufferd = jtag_tap_tdoDr; // @[JtagTap.scala 101:22]
        end
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    jtag_tap_tdoDr = 1'b0; // @[JtagTap.scala 79:15]
    if(jtag_idcodeArea_ctrl_enable) begin
      jtag_tap_tdoDr = jtag_idcodeArea_ctrl_tdo; // @[JtagTap.scala 113:31]
    end
    if(jtag_writeArea_ctrl_enable) begin
      jtag_tap_tdoDr = jtag_writeArea_ctrl_tdo; // @[JtagTap.scala 113:31]
    end
    if(jtag_readArea_ctrl_enable) begin
      jtag_tap_tdoDr = jtag_readArea_ctrl_tdo; // @[JtagTap.scala 113:31]
    end
  end

  assign jtag_tap_tdoIr = jtag_tap_instructionShift[0]; // @[BaseType.scala 305:24]
  assign jtag_tap_isBypass = ($signed(_zz_jtag_tap_isBypass) == $signed(_zz_jtag_tap_isBypass_1)); // @[BaseType.scala 305:24]
  assign io_jtag_tdo = jtag_tap_tdoUnbufferd_regNext; // @[JtagTap.scala 83:12]
  assign jtag_idcodeArea_ctrl_tdo = jtag_idcodeArea_shifter[0]; // @[JtagTapInstructions.scala 143:12]
  assign jtag_idcodeArea_ctrl_tdi = io_jtag_tdi; // @[JtagTap.scala 107:18]
  assign jtag_idcodeArea_ctrl_enable = (jtag_tap_instruction == 4'b0001); // @[JtagTap.scala 108:18]
  assign jtag_idcodeArea_ctrl_capture = (jtag_tap_fsm_state == JtagState_DR_CAPTURE); // @[JtagTap.scala 109:18]
  assign jtag_idcodeArea_ctrl_shift = (jtag_tap_fsm_state == JtagState_DR_SHIFT); // @[JtagTap.scala 110:18]
  assign jtag_idcodeArea_ctrl_update = (jtag_tap_fsm_state == JtagState_DR_UPDATE); // @[JtagTap.scala 111:18]
  assign jtag_idcodeArea_ctrl_reset = (jtag_tap_fsm_state == JtagState_RESET); // @[JtagTap.scala 112:18]
  assign when_JtagTap_l120 = (jtag_tap_fsm_state == JtagState_RESET); // @[BaseType.scala 305:24]
  assign jtag_writeArea_source_valid = jtag_writeArea_valid; // @[JtagTapInstructions.scala 154:16]
  assign jtag_writeArea_source_payload_last = (! (jtag_writeArea_ctrl_enable && jtag_writeArea_ctrl_shift)); // @[JtagTapInstructions.scala 155:15]
  assign jtag_writeArea_source_payload_fragment[0] = jtag_writeArea_data; // @[JtagTapInstructions.scala 156:23]
  assign system_cmd_valid = flowCCByToggle_1_io_output_valid; // @[Flow.scala 94:11]
  assign system_cmd_payload_last = flowCCByToggle_1_io_output_payload_last; // @[Flow.scala 95:13]
  assign system_cmd_payload_fragment = flowCCByToggle_1_io_output_payload_fragment; // @[Flow.scala 95:13]
  assign jtag_writeArea_ctrl_tdo = 1'b0; // @[JtagTapInstructions.scala 160:12]
  assign jtag_writeArea_ctrl_tdi = io_jtag_tdi; // @[JtagTap.scala 107:18]
  assign jtag_writeArea_ctrl_enable = (jtag_tap_instruction == 4'b0010); // @[JtagTap.scala 108:18]
  assign jtag_writeArea_ctrl_capture = (jtag_tap_fsm_state == JtagState_DR_CAPTURE); // @[JtagTap.scala 109:18]
  assign jtag_writeArea_ctrl_shift = (jtag_tap_fsm_state == JtagState_DR_SHIFT); // @[JtagTap.scala 110:18]
  assign jtag_writeArea_ctrl_update = (jtag_tap_fsm_state == JtagState_DR_UPDATE); // @[JtagTap.scala 111:18]
  assign jtag_writeArea_ctrl_reset = (jtag_tap_fsm_state == JtagState_RESET); // @[JtagTap.scala 112:18]
  assign jtag_readArea_ctrl_tdo = jtag_readArea_full_shifter[0]; // @[JtagTapInstructions.scala 88:14]
  assign jtag_readArea_ctrl_tdi = io_jtag_tdi; // @[JtagTap.scala 107:18]
  assign jtag_readArea_ctrl_enable = (jtag_tap_instruction == 4'b0011); // @[JtagTap.scala 108:18]
  assign jtag_readArea_ctrl_capture = (jtag_tap_fsm_state == JtagState_DR_CAPTURE); // @[JtagTap.scala 109:18]
  assign jtag_readArea_ctrl_shift = (jtag_tap_fsm_state == JtagState_DR_SHIFT); // @[JtagTap.scala 110:18]
  assign jtag_readArea_ctrl_update = (jtag_tap_fsm_state == JtagState_DR_UPDATE); // @[JtagTap.scala 111:18]
  assign jtag_readArea_ctrl_reset = (jtag_tap_fsm_state == JtagState_RESET); // @[JtagTap.scala 112:18]
  always @(posedge io_mainClk) begin
    if(io_remote_cmd_valid) begin
      system_rsp_valid <= 1'b0; // @[SystemDebugger.scala 71:17]
    end
    if(io_remote_rsp_fire) begin
      system_rsp_valid <= 1'b1; // @[SystemDebugger.scala 74:17]
      system_rsp_payload_error <= io_remote_rsp_payload_error; // @[SystemDebugger.scala 75:19]
      system_rsp_payload_data <= io_remote_rsp_payload_data; // @[SystemDebugger.scala 75:19]
    end
  end

  always @(posedge io_jtag_tck) begin
    jtag_tap_fsm_state <= jtag_tap_fsm_stateNext; // @[Reg.scala 39:30]
    jtag_tap_bypass <= io_jtag_tdi; // @[Reg.scala 39:30]
    case(jtag_tap_fsm_state)
      JtagState_IR_CAPTURE : begin
        jtag_tap_instructionShift <= {2'd0, _zz_jtag_tap_instructionShift}; // @[JtagTap.scala 87:24]
      end
      JtagState_IR_SHIFT : begin
        jtag_tap_instructionShift <= ({io_jtag_tdi,jtag_tap_instructionShift} >>> 1); // @[JtagTap.scala 90:24]
      end
      JtagState_IR_UPDATE : begin
        jtag_tap_instruction <= jtag_tap_instructionShift; // @[JtagTap.scala 94:19]
      end
      JtagState_DR_SHIFT : begin
        jtag_tap_instructionShift <= ({io_jtag_tdi,jtag_tap_instructionShift} >>> 1); // @[JtagTap.scala 97:24]
      end
      default : begin
      end
    endcase
    if(jtag_idcodeArea_ctrl_enable) begin
      if(jtag_idcodeArea_ctrl_shift) begin
        jtag_idcodeArea_shifter <= ({jtag_idcodeArea_ctrl_tdi,jtag_idcodeArea_shifter} >>> 1); // @[JtagTapInstructions.scala 135:15]
      end
    end
    if(jtag_idcodeArea_ctrl_capture) begin
      jtag_idcodeArea_shifter <= 32'h10001fff; // @[JtagTapInstructions.scala 140:13]
    end
    if(when_JtagTap_l120) begin
      jtag_tap_instruction <= 4'b0001; // @[JtagTap.scala 120:54]
    end
    jtag_writeArea_valid <= (jtag_writeArea_ctrl_enable && jtag_writeArea_ctrl_shift); // @[Reg.scala 39:30]
    jtag_writeArea_data <= jtag_writeArea_ctrl_tdi; // @[Reg.scala 39:30]
    if(jtag_readArea_ctrl_enable) begin
      if(jtag_readArea_ctrl_capture) begin
        jtag_readArea_full_shifter <= {{system_rsp_payload_data,system_rsp_payload_error},system_rsp_valid}; // @[JtagTapInstructions.scala 81:17]
      end
      if(jtag_readArea_ctrl_shift) begin
        jtag_readArea_full_shifter <= ({jtag_readArea_ctrl_tdi,jtag_readArea_full_shifter} >>> 1); // @[JtagTapInstructions.scala 85:17]
      end
    end
  end

  always @(negedge io_jtag_tck) begin
    jtag_tap_tdoUnbufferd_regNext <= jtag_tap_tdoUnbufferd; // @[Reg.scala 39:30]
  end


endmodule

module VexRiscv (
  output              iBus_cmd_valid,
  input               iBus_cmd_ready,
  output     [31:0]   iBus_cmd_payload_pc,
  input               iBus_rsp_valid,
  input               iBus_rsp_payload_error,
  input      [31:0]   iBus_rsp_payload_inst,
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
  output              dBus_cmd_valid,
  input               dBus_cmd_ready,
  output              dBus_cmd_payload_wr,
  output     [31:0]   dBus_cmd_payload_address,
  output     [31:0]   dBus_cmd_payload_data,
  output     [1:0]    dBus_cmd_payload_size,
  input               dBus_rsp_ready,
  input               dBus_rsp_error,
  input      [31:0]   dBus_rsp_data,
  input               io_mainClk,
  input               resetCtrl_systemReset,
  input               resetCtrl_mainClkReset
);
  localparam BranchCtrlEnum_INC = 2'd0;
  localparam BranchCtrlEnum_B = 2'd1;
  localparam BranchCtrlEnum_JAL = 2'd2;
  localparam BranchCtrlEnum_JALR = 2'd3;
  localparam ShiftCtrlEnum_DISABLE_1 = 2'd0;
  localparam ShiftCtrlEnum_SLL_1 = 2'd1;
  localparam ShiftCtrlEnum_SRL_1 = 2'd2;
  localparam ShiftCtrlEnum_SRA_1 = 2'd3;
  localparam AluBitwiseCtrlEnum_XOR_1 = 2'd0;
  localparam AluBitwiseCtrlEnum_OR_1 = 2'd1;
  localparam AluBitwiseCtrlEnum_AND_1 = 2'd2;
  localparam AluCtrlEnum_ADD_SUB = 2'd0;
  localparam AluCtrlEnum_SLT_SLTU = 2'd1;
  localparam AluCtrlEnum_BITWISE = 2'd2;
  localparam EnvCtrlEnum_NONE = 1'd0;
  localparam EnvCtrlEnum_XRET = 1'd1;
  localparam Src2CtrlEnum_RS = 2'd0;
  localparam Src2CtrlEnum_IMI = 2'd1;
  localparam Src2CtrlEnum_IMS = 2'd2;
  localparam Src2CtrlEnum_PC = 2'd3;
  localparam Src1CtrlEnum_RS = 2'd0;
  localparam Src1CtrlEnum_IMU = 2'd1;
  localparam Src1CtrlEnum_PC_INCREMENT = 2'd2;
  localparam Src1CtrlEnum_URS1 = 2'd3;

  wire                IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_ready;
  reg        [31:0]   _zz_RegFilePlugin_regFile_port0;
  reg        [31:0]   _zz_RegFilePlugin_regFile_port1;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_c_io_push_ready;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_valid;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_payload_error;
  wire       [31:0]   IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_payload_inst;
  wire       [0:0]    IBusSimplePlugin_rspJoin_rspBuffer_c_io_occupancy;
  wire       [1:0]    _zz_IBusSimplePlugin_jump_pcLoad_payload_1;
  wire       [1:0]    _zz_IBusSimplePlugin_jump_pcLoad_payload_2;
  wire       [31:0]   _zz_IBusSimplePlugin_fetchPc_pc;
  wire       [2:0]    _zz_IBusSimplePlugin_fetchPc_pc_1;
  wire       [2:0]    _zz_IBusSimplePlugin_pending_next;
  wire       [2:0]    _zz_IBusSimplePlugin_pending_next_1;
  wire       [0:0]    _zz_IBusSimplePlugin_pending_next_2;
  wire       [2:0]    _zz_IBusSimplePlugin_pending_next_3;
  wire       [0:0]    _zz_IBusSimplePlugin_pending_next_4;
  wire       [2:0]    _zz_IBusSimplePlugin_rspJoin_rspBuffer_discardCounter;
  wire       [0:0]    _zz_IBusSimplePlugin_rspJoin_rspBuffer_discardCounter_1;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_1;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_2;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_3;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_4;
  wire       [1:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_5;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_6;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_7;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_8;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_9;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_10;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_11;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_12;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_13;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_14;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_15;
  wire       [18:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_16;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_17;
  wire       [1:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_18;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_19;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_20;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_21;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_22;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_23;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_24;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_25;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_26;
  wire       [14:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_27;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_28;
  wire       [1:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_29;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_30;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_31;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_32;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_33;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_34;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_35;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_36;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_37;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_38;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_39;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_40;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_41;
  wire       [10:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_42;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_43;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_44;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_45;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_46;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_47;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_48;
  wire       [4:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_49;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_50;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_51;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_52;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_53;
  wire       [1:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_54;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_55;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_56;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_57;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_58;
  wire       [6:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_59;
  wire       [1:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_60;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_61;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_62;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_63;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_64;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_65;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_66;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_67;
  wire       [2:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_68;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_69;
  wire       [3:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_70;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_71;
  wire       [31:0]   _zz__zz_decode_SRC_LESS_UNSIGNED_72;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_73;
  wire                _zz__zz_decode_SRC_LESS_UNSIGNED_74;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_75;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_76;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_77;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_78;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_79;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_80;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_81;
  wire       [0:0]    _zz__zz_decode_SRC_LESS_UNSIGNED_82;
  wire                _zz_RegFilePlugin_regFile_port;
  wire                _zz_decode_RegFilePlugin_rs1Data;
  wire                _zz_RegFilePlugin_regFile_port_1;
  wire                _zz_decode_RegFilePlugin_rs2Data;
  wire       [0:0]    _zz__zz_execute_REGFILE_WRITE_DATA;
  wire       [2:0]    _zz__zz_decode_SRC1;
  wire       [4:0]    _zz__zz_decode_SRC1_1;
  wire       [11:0]   _zz__zz_decode_SRC2_2;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_1;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_2;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_3;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_4;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_5;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_6;
  wire       [31:0]   _zz__zz_decode_RS2_3;
  wire       [32:0]   _zz__zz_decode_RS2_3_1;
  wire       [19:0]   _zz__zz_execute_BranchPlugin_branch_src2;
  wire       [11:0]   _zz__zz_execute_BranchPlugin_branch_src2_4;
  wire       [31:0]   memory_MEMORY_READ_DATA;
  wire       [31:0]   execute_BRANCH_CALC;
  wire                execute_BRANCH_DO;
  wire       [31:0]   writeBack_REGFILE_WRITE_DATA;
  wire       [31:0]   execute_REGFILE_WRITE_DATA;
  wire       [1:0]    memory_MEMORY_ADDRESS_LOW;
  wire       [1:0]    execute_MEMORY_ADDRESS_LOW;
  wire                decode_DO_EBREAK;
  wire       [31:0]   decode_SRC2;
  wire       [31:0]   decode_SRC1;
  wire                decode_SRC2_FORCE_ZERO;
  wire       [1:0]    decode_BRANCH_CTRL;
  wire       [1:0]    _zz_decode_BRANCH_CTRL;
  wire       [1:0]    _zz_decode_to_execute_BRANCH_CTRL;
  wire       [1:0]    _zz_decode_to_execute_BRANCH_CTRL_1;
  wire       [1:0]    decode_SHIFT_CTRL;
  wire       [1:0]    _zz_decode_SHIFT_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SHIFT_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SHIFT_CTRL_1;
  wire       [1:0]    decode_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_decode_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_BITWISE_CTRL_1;
  wire                decode_SRC_LESS_UNSIGNED;
  wire       [1:0]    decode_ALU_CTRL;
  wire       [1:0]    _zz_decode_ALU_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_CTRL_1;
  wire       [0:0]    _zz_memory_to_writeBack_ENV_CTRL;
  wire       [0:0]    _zz_memory_to_writeBack_ENV_CTRL_1;
  wire       [0:0]    _zz_execute_to_memory_ENV_CTRL;
  wire       [0:0]    _zz_execute_to_memory_ENV_CTRL_1;
  wire       [0:0]    decode_ENV_CTRL;
  wire       [0:0]    _zz_decode_ENV_CTRL;
  wire       [0:0]    _zz_decode_to_execute_ENV_CTRL;
  wire       [0:0]    _zz_decode_to_execute_ENV_CTRL_1;
  wire                decode_IS_CSR;
  wire                decode_MEMORY_STORE;
  wire                execute_BYPASSABLE_MEMORY_STAGE;
  wire                decode_BYPASSABLE_MEMORY_STAGE;
  wire                decode_BYPASSABLE_EXECUTE_STAGE;
  wire                decode_MEMORY_ENABLE;
  wire                decode_CSR_READ_OPCODE;
  wire                decode_CSR_WRITE_OPCODE;
  wire       [31:0]   writeBack_FORMAL_PC_NEXT;
  wire       [31:0]   memory_FORMAL_PC_NEXT;
  wire       [31:0]   execute_FORMAL_PC_NEXT;
  wire       [31:0]   decode_FORMAL_PC_NEXT;
  wire       [31:0]   memory_PC;
  wire                execute_DO_EBREAK;
  wire                decode_IS_EBREAK;
  wire       [31:0]   memory_BRANCH_CALC;
  wire                memory_BRANCH_DO;
  wire       [31:0]   execute_PC;
  wire       [31:0]   execute_RS1;
  wire       [1:0]    execute_BRANCH_CTRL;
  wire       [1:0]    _zz_execute_BRANCH_CTRL;
  wire                decode_RS2_USE;
  wire                decode_RS1_USE;
  wire                execute_REGFILE_WRITE_VALID;
  wire                execute_BYPASSABLE_EXECUTE_STAGE;
  wire       [31:0]   _zz_decode_RS2;
  wire                memory_REGFILE_WRITE_VALID;
  wire       [31:0]   memory_INSTRUCTION;
  wire                memory_BYPASSABLE_MEMORY_STAGE;
  wire                writeBack_REGFILE_WRITE_VALID;
  reg        [31:0]   decode_RS2;
  reg        [31:0]   decode_RS1;
  wire       [31:0]   memory_REGFILE_WRITE_DATA;
  wire       [1:0]    execute_SHIFT_CTRL;
  wire       [1:0]    _zz_execute_SHIFT_CTRL;
  wire                execute_SRC_LESS_UNSIGNED;
  wire                execute_SRC2_FORCE_ZERO;
  wire                execute_SRC_USE_SUB_LESS;
  wire       [31:0]   _zz_decode_to_execute_PC;
  wire       [31:0]   _zz_decode_to_execute_RS2;
  wire       [1:0]    decode_SRC2_CTRL;
  wire       [1:0]    _zz_decode_SRC2_CTRL;
  wire       [31:0]   _zz_decode_to_execute_RS1;
  wire       [1:0]    decode_SRC1_CTRL;
  wire       [1:0]    _zz_decode_SRC1_CTRL;
  wire                decode_SRC_USE_SUB_LESS;
  wire                decode_SRC_ADD_ZERO;
  wire       [31:0]   execute_SRC_ADD_SUB;
  wire                execute_SRC_LESS;
  wire       [1:0]    execute_ALU_CTRL;
  wire       [1:0]    _zz_execute_ALU_CTRL;
  wire       [31:0]   execute_SRC2;
  wire       [1:0]    execute_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_execute_ALU_BITWISE_CTRL;
  wire       [31:0]   _zz_lastStageRegFileWrite_payload_address;
  wire                _zz_lastStageRegFileWrite_valid;
  reg                 _zz_1;
  wire       [31:0]   decode_INSTRUCTION_ANTICIPATED;
  reg                 decode_REGFILE_WRITE_VALID;
  wire       [1:0]    _zz_decode_BRANCH_CTRL_1;
  wire       [1:0]    _zz_decode_SHIFT_CTRL_1;
  wire       [1:0]    _zz_decode_ALU_BITWISE_CTRL_1;
  wire       [1:0]    _zz_decode_ALU_CTRL_1;
  wire       [0:0]    _zz_decode_ENV_CTRL_1;
  wire       [1:0]    _zz_decode_SRC2_CTRL_1;
  wire       [1:0]    _zz_decode_SRC1_CTRL_1;
  reg        [31:0]   _zz_decode_RS2_1;
  wire       [31:0]   execute_SRC1;
  wire                execute_CSR_READ_OPCODE;
  wire                execute_CSR_WRITE_OPCODE;
  wire                execute_IS_CSR;
  wire       [0:0]    memory_ENV_CTRL;
  wire       [0:0]    _zz_memory_ENV_CTRL;
  wire       [0:0]    execute_ENV_CTRL;
  wire       [0:0]    _zz_execute_ENV_CTRL;
  wire       [0:0]    writeBack_ENV_CTRL;
  wire       [0:0]    _zz_writeBack_ENV_CTRL;
  reg        [31:0]   _zz_decode_RS2_2;
  wire                writeBack_MEMORY_ENABLE;
  wire       [1:0]    writeBack_MEMORY_ADDRESS_LOW;
  wire       [31:0]   writeBack_MEMORY_READ_DATA;
  wire                memory_MEMORY_STORE;
  wire                memory_MEMORY_ENABLE;
  wire       [31:0]   execute_SRC_ADD;
  wire       [31:0]   execute_RS2;
  wire       [31:0]   execute_INSTRUCTION;
  wire                execute_MEMORY_STORE;
  wire                execute_MEMORY_ENABLE;
  wire                execute_ALIGNEMENT_FAULT;
  reg        [31:0]   _zz_memory_to_writeBack_FORMAL_PC_NEXT;
  wire       [31:0]   decode_PC;
  wire       [31:0]   decode_INSTRUCTION;
  wire       [31:0]   writeBack_PC;
  wire       [31:0]   writeBack_INSTRUCTION;
  reg                 decode_arbitration_haltItself;
  reg                 decode_arbitration_haltByOther;
  reg                 decode_arbitration_removeIt;
  wire                decode_arbitration_flushIt;
  wire                decode_arbitration_flushNext;
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
  wire                writeBack_arbitration_haltItself;
  wire                writeBack_arbitration_haltByOther;
  reg                 writeBack_arbitration_removeIt;
  wire                writeBack_arbitration_flushIt;
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
  reg                 IBusSimplePlugin_fetcherHalt;
  wire                IBusSimplePlugin_forceNoDecodeCond;
  reg                 IBusSimplePlugin_incomingInstruction;
  wire                IBusSimplePlugin_pcValids_0;
  wire                IBusSimplePlugin_pcValids_1;
  wire                IBusSimplePlugin_pcValids_2;
  wire                IBusSimplePlugin_pcValids_3;
  wire       [31:0]   CsrPlugin_csrMapping_readDataSignal;
  wire       [31:0]   CsrPlugin_csrMapping_readDataInit;
  wire       [31:0]   CsrPlugin_csrMapping_writeDataSignal;
  wire                CsrPlugin_csrMapping_allowCsrSignal;
  wire                CsrPlugin_csrMapping_hazardFree;
  wire                CsrPlugin_inWfi /* verilator public */ ;
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
  reg                 CsrPlugin_allowInterrupts;
  reg                 CsrPlugin_allowException;
  reg                 CsrPlugin_allowEbreakException;
  wire                CsrPlugin_xretAwayFromMachine;
  wire                BranchPlugin_jumpInterface_valid;
  wire       [31:0]   BranchPlugin_jumpInterface_payload;
  reg                 BranchPlugin_inDebugNoFetchFlag;
  reg                 IBusSimplePlugin_injectionPort_valid;
  reg                 IBusSimplePlugin_injectionPort_ready;
  wire       [31:0]   IBusSimplePlugin_injectionPort_payload;
  wire                IBusSimplePlugin_externalFlush;
  wire                IBusSimplePlugin_jump_pcLoad_valid;
  wire       [31:0]   IBusSimplePlugin_jump_pcLoad_payload;
  wire       [1:0]    _zz_IBusSimplePlugin_jump_pcLoad_payload;
  wire                IBusSimplePlugin_fetchPc_output_valid;
  wire                IBusSimplePlugin_fetchPc_output_ready;
  wire       [31:0]   IBusSimplePlugin_fetchPc_output_payload;
  reg        [31:0]   IBusSimplePlugin_fetchPc_pcReg /* verilator public */ ;
  reg                 IBusSimplePlugin_fetchPc_correction;
  reg                 IBusSimplePlugin_fetchPc_correctionReg;
  wire                IBusSimplePlugin_fetchPc_output_fire;
  wire                IBusSimplePlugin_fetchPc_corrected;
  reg                 IBusSimplePlugin_fetchPc_pcRegPropagate;
  reg                 IBusSimplePlugin_fetchPc_booted;
  reg                 IBusSimplePlugin_fetchPc_inc;
  wire                when_Fetcher_l134;
  wire                IBusSimplePlugin_fetchPc_output_fire_1;
  wire                when_Fetcher_l134_1;
  reg        [31:0]   IBusSimplePlugin_fetchPc_pc;
  reg                 IBusSimplePlugin_fetchPc_flushed;
  wire                when_Fetcher_l161;
  wire                IBusSimplePlugin_iBusRsp_redoFetch;
  wire                IBusSimplePlugin_iBusRsp_stages_0_input_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_0_input_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_0_input_payload;
  wire                IBusSimplePlugin_iBusRsp_stages_0_output_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_0_output_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_0_output_payload;
  wire                IBusSimplePlugin_iBusRsp_stages_0_halt;
  wire                IBusSimplePlugin_iBusRsp_stages_1_input_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_1_input_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_1_input_payload;
  wire                IBusSimplePlugin_iBusRsp_stages_1_output_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_1_output_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_1_output_payload;
  reg                 IBusSimplePlugin_iBusRsp_stages_1_halt;
  wire                IBusSimplePlugin_iBusRsp_stages_2_input_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_2_input_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_2_input_payload;
  wire                IBusSimplePlugin_iBusRsp_stages_2_output_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_2_output_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_2_output_payload;
  wire                IBusSimplePlugin_iBusRsp_stages_2_halt;
  wire                _zz_IBusSimplePlugin_iBusRsp_stages_0_input_ready;
  wire                _zz_IBusSimplePlugin_iBusRsp_stages_1_input_ready;
  wire                _zz_IBusSimplePlugin_iBusRsp_stages_2_input_ready;
  wire                IBusSimplePlugin_iBusRsp_flush;
  wire                _zz_IBusSimplePlugin_iBusRsp_stages_0_output_ready;
  wire                _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid;
  reg                 _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid_1;
  wire                IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid;
  wire                IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_payload;
  reg                 _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid;
  reg        [31:0]   _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_payload;
  reg                 IBusSimplePlugin_iBusRsp_readyForError;
  wire                IBusSimplePlugin_iBusRsp_output_valid;
  wire                IBusSimplePlugin_iBusRsp_output_ready;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_output_payload_pc;
  wire                IBusSimplePlugin_iBusRsp_output_payload_rsp_error;
  wire       [31:0]   IBusSimplePlugin_iBusRsp_output_payload_rsp_inst;
  wire                IBusSimplePlugin_iBusRsp_output_payload_isRvc;
  wire                when_Fetcher_l243;
  wire                IBusSimplePlugin_injector_decodeInput_valid;
  wire                IBusSimplePlugin_injector_decodeInput_ready;
  wire       [31:0]   IBusSimplePlugin_injector_decodeInput_payload_pc;
  wire                IBusSimplePlugin_injector_decodeInput_payload_rsp_error;
  wire       [31:0]   IBusSimplePlugin_injector_decodeInput_payload_rsp_inst;
  wire                IBusSimplePlugin_injector_decodeInput_payload_isRvc;
  reg                 _zz_IBusSimplePlugin_injector_decodeInput_valid;
  reg        [31:0]   _zz_IBusSimplePlugin_injector_decodeInput_payload_pc;
  reg                 _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_error;
  reg        [31:0]   _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_inst;
  reg                 _zz_IBusSimplePlugin_injector_decodeInput_payload_isRvc;
  wire                when_Fetcher_l323;
  reg                 IBusSimplePlugin_injector_nextPcCalc_valids_0;
  wire                when_Fetcher_l332;
  reg                 IBusSimplePlugin_injector_nextPcCalc_valids_1;
  wire                when_Fetcher_l332_1;
  reg                 IBusSimplePlugin_injector_nextPcCalc_valids_2;
  wire                when_Fetcher_l332_2;
  reg                 IBusSimplePlugin_injector_nextPcCalc_valids_3;
  wire                when_Fetcher_l332_3;
  reg                 IBusSimplePlugin_injector_nextPcCalc_valids_4;
  wire                when_Fetcher_l332_4;
  reg                 IBusSimplePlugin_injector_nextPcCalc_valids_5;
  wire                when_Fetcher_l332_5;
  reg        [31:0]   IBusSimplePlugin_injector_formal_rawInDecode;
  wire                IBusSimplePlugin_cmd_valid;
  wire                IBusSimplePlugin_cmd_ready;
  wire       [31:0]   IBusSimplePlugin_cmd_payload_pc;
  wire                IBusSimplePlugin_pending_inc;
  wire                IBusSimplePlugin_pending_dec;
  reg        [2:0]    IBusSimplePlugin_pending_value;
  wire       [2:0]    IBusSimplePlugin_pending_next;
  wire                IBusSimplePlugin_cmdFork_canEmit;
  wire                when_IBusSimplePlugin_l305;
  wire                IBusSimplePlugin_cmd_fire;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_output_valid;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_output_ready;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_output_payload_error;
  wire       [31:0]   IBusSimplePlugin_rspJoin_rspBuffer_output_payload_inst;
  reg        [2:0]    IBusSimplePlugin_rspJoin_rspBuffer_discardCounter;
  wire                iBus_rsp_toStream_valid;
  wire                iBus_rsp_toStream_ready;
  wire                iBus_rsp_toStream_payload_error;
  wire       [31:0]   iBus_rsp_toStream_payload_inst;
  wire                IBusSimplePlugin_rspJoin_rspBuffer_flush;
  wire                system_cpu_IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_fire;
  wire       [31:0]   IBusSimplePlugin_rspJoin_fetchRsp_pc;
  reg                 IBusSimplePlugin_rspJoin_fetchRsp_rsp_error;
  wire       [31:0]   IBusSimplePlugin_rspJoin_fetchRsp_rsp_inst;
  wire                IBusSimplePlugin_rspJoin_fetchRsp_isRvc;
  wire                when_IBusSimplePlugin_l376;
  wire                IBusSimplePlugin_rspJoin_join_valid;
  wire                IBusSimplePlugin_rspJoin_join_ready;
  wire       [31:0]   IBusSimplePlugin_rspJoin_join_payload_pc;
  wire                IBusSimplePlugin_rspJoin_join_payload_rsp_error;
  wire       [31:0]   IBusSimplePlugin_rspJoin_join_payload_rsp_inst;
  wire                IBusSimplePlugin_rspJoin_join_payload_isRvc;
  wire                IBusSimplePlugin_rspJoin_exceptionDetected;
  wire                IBusSimplePlugin_rspJoin_join_fire;
  wire                IBusSimplePlugin_rspJoin_join_fire_1;
  wire                _zz_IBusSimplePlugin_iBusRsp_output_valid;
  wire                _zz_dBus_cmd_valid;
  reg                 execute_DBusSimplePlugin_skipCmd;
  reg        [31:0]   _zz_dBus_cmd_payload_data;
  wire                when_DBusSimplePlugin_l428;
  reg        [3:0]    _zz_execute_DBusSimplePlugin_formalMask;
  wire       [3:0]    execute_DBusSimplePlugin_formalMask;
  wire                when_DBusSimplePlugin_l482;
  reg        [31:0]   writeBack_DBusSimplePlugin_rspShifted;
  wire       [1:0]    switch_Misc_l226;
  wire                _zz_writeBack_DBusSimplePlugin_rspFormated;
  reg        [31:0]   _zz_writeBack_DBusSimplePlugin_rspFormated_1;
  wire                _zz_writeBack_DBusSimplePlugin_rspFormated_2;
  reg        [31:0]   _zz_writeBack_DBusSimplePlugin_rspFormated_3;
  reg        [31:0]   writeBack_DBusSimplePlugin_rspFormated;
  wire                when_DBusSimplePlugin_l558;
  wire       [1:0]    CsrPlugin_misa_base;
  wire       [25:0]   CsrPlugin_misa_extensions;
  wire       [1:0]    CsrPlugin_mtvec_mode;
  wire       [29:0]   CsrPlugin_mtvec_base;
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
  reg                 CsrPlugin_mcause_interrupt;
  reg        [3:0]    CsrPlugin_mcause_exceptionCode;
  reg        [31:0]   CsrPlugin_mtval;
  reg        [63:0]   CsrPlugin_mcycle;
  reg        [63:0]   CsrPlugin_minstret;
  wire                _zz_when_CsrPlugin_l1222;
  wire                _zz_when_CsrPlugin_l1222_1;
  wire                _zz_when_CsrPlugin_l1222_2;
  reg                 CsrPlugin_interrupt_valid;
  reg        [3:0]    CsrPlugin_interrupt_code /* verilator public */ ;
  reg        [1:0]    CsrPlugin_interrupt_targetPrivilege;
  wire                when_CsrPlugin_l1216;
  wire                when_CsrPlugin_l1222;
  wire                when_CsrPlugin_l1222_1;
  wire                when_CsrPlugin_l1222_2;
  wire                CsrPlugin_exception;
  wire                CsrPlugin_lastStageWasWfi;
  reg                 CsrPlugin_pipelineLiberator_pcValids_0;
  reg                 CsrPlugin_pipelineLiberator_pcValids_1;
  reg                 CsrPlugin_pipelineLiberator_pcValids_2;
  wire                CsrPlugin_pipelineLiberator_active;
  wire                when_CsrPlugin_l1255;
  wire                when_CsrPlugin_l1255_1;
  wire                when_CsrPlugin_l1255_2;
  wire                when_CsrPlugin_l1260;
  reg                 CsrPlugin_pipelineLiberator_done;
  wire                CsrPlugin_interruptJump /* verilator public */ ;
  reg                 CsrPlugin_hadException /* verilator public */ ;
  wire       [1:0]    CsrPlugin_targetPrivilege;
  wire       [3:0]    CsrPlugin_trapCause;
  wire                CsrPlugin_trapCauseEbreakDebug;
  reg        [1:0]    CsrPlugin_xtvec_mode;
  reg        [29:0]   CsrPlugin_xtvec_base;
  wire                CsrPlugin_trapEnterDebug;
  wire                when_CsrPlugin_l1310;
  wire                when_CsrPlugin_l1318;
  wire                when_CsrPlugin_l1376;
  wire       [1:0]    switch_CsrPlugin_l1380;
  reg                 execute_CsrPlugin_wfiWake;
  wire                when_CsrPlugin_l1447;
  wire                execute_CsrPlugin_blockedBySideEffects;
  reg                 execute_CsrPlugin_illegalAccess;
  reg                 execute_CsrPlugin_illegalInstruction;
  wire                when_CsrPlugin_l1467;
  wire                when_CsrPlugin_l1468;
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
  wire       [25:0]   _zz_decode_SRC_LESS_UNSIGNED;
  wire                _zz_decode_SRC_LESS_UNSIGNED_1;
  wire                _zz_decode_SRC_LESS_UNSIGNED_2;
  wire                _zz_decode_SRC_LESS_UNSIGNED_3;
  wire                _zz_decode_SRC_LESS_UNSIGNED_4;
  wire                _zz_decode_SRC_LESS_UNSIGNED_5;
  wire                _zz_decode_SRC_LESS_UNSIGNED_6;
  wire       [1:0]    _zz_decode_SRC1_CTRL_2;
  wire       [1:0]    _zz_decode_SRC2_CTRL_2;
  wire       [0:0]    _zz_decode_ENV_CTRL_2;
  wire       [1:0]    _zz_decode_ALU_CTRL_2;
  wire       [1:0]    _zz_decode_ALU_BITWISE_CTRL_2;
  wire       [1:0]    _zz_decode_SHIFT_CTRL_2;
  wire       [1:0]    _zz_decode_BRANCH_CTRL_2;
  wire                when_RegFilePlugin_l63;
  wire       [4:0]    decode_RegFilePlugin_regFileReadAddress1;
  wire       [4:0]    decode_RegFilePlugin_regFileReadAddress2;
  wire       [31:0]   decode_RegFilePlugin_rs1Data;
  wire       [31:0]   decode_RegFilePlugin_rs2Data;
  reg                 lastStageRegFileWrite_valid /* verilator public */ ;
  reg        [4:0]    lastStageRegFileWrite_payload_address /* verilator public */ ;
  reg        [31:0]   lastStageRegFileWrite_payload_data /* verilator public */ ;
  reg                 _zz_2;
  reg        [31:0]   execute_IntAluPlugin_bitwise;
  reg        [31:0]   _zz_execute_REGFILE_WRITE_DATA;
  reg        [31:0]   _zz_decode_SRC1;
  wire                _zz_decode_SRC2;
  reg        [19:0]   _zz_decode_SRC2_1;
  wire                _zz_decode_SRC2_2;
  reg        [19:0]   _zz_decode_SRC2_3;
  reg        [31:0]   _zz_decode_SRC2_4;
  reg        [31:0]   execute_SrcPlugin_addSub;
  wire                execute_SrcPlugin_less;
  reg                 execute_LightShifterPlugin_isActive;
  wire                execute_LightShifterPlugin_isShift;
  reg        [4:0]    execute_LightShifterPlugin_amplitudeReg;
  wire       [4:0]    execute_LightShifterPlugin_amplitude;
  wire       [31:0]   execute_LightShifterPlugin_shiftInput;
  wire                execute_LightShifterPlugin_done;
  wire                when_ShiftPlugins_l169;
  reg        [31:0]   _zz_decode_RS2_3;
  wire                when_ShiftPlugins_l175;
  wire                when_ShiftPlugins_l184;
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
  wire                execute_BranchPlugin_eq;
  wire       [2:0]    switch_Misc_l226_2;
  reg                 _zz_execute_BRANCH_DO;
  reg                 _zz_execute_BRANCH_DO_1;
  wire       [31:0]   execute_BranchPlugin_branch_src1;
  wire                _zz_execute_BranchPlugin_branch_src2;
  reg        [10:0]   _zz_execute_BranchPlugin_branch_src2_1;
  wire                _zz_execute_BranchPlugin_branch_src2_2;
  reg        [19:0]   _zz_execute_BranchPlugin_branch_src2_3;
  wire                _zz_execute_BranchPlugin_branch_src2_4;
  reg        [18:0]   _zz_execute_BranchPlugin_branch_src2_5;
  reg        [31:0]   _zz_execute_BranchPlugin_branch_src2_6;
  wire       [31:0]   execute_BranchPlugin_branch_src2;
  wire       [31:0]   execute_BranchPlugin_branchAdder;
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
  reg                 decode_to_execute_CSR_WRITE_OPCODE;
  wire                when_Pipeline_l124_10;
  reg                 decode_to_execute_CSR_READ_OPCODE;
  wire                when_Pipeline_l124_11;
  reg                 decode_to_execute_SRC_USE_SUB_LESS;
  wire                when_Pipeline_l124_12;
  reg                 decode_to_execute_MEMORY_ENABLE;
  wire                when_Pipeline_l124_13;
  reg                 execute_to_memory_MEMORY_ENABLE;
  wire                when_Pipeline_l124_14;
  reg                 memory_to_writeBack_MEMORY_ENABLE;
  wire                when_Pipeline_l124_15;
  reg                 decode_to_execute_REGFILE_WRITE_VALID;
  wire                when_Pipeline_l124_16;
  reg                 execute_to_memory_REGFILE_WRITE_VALID;
  wire                when_Pipeline_l124_17;
  reg                 memory_to_writeBack_REGFILE_WRITE_VALID;
  wire                when_Pipeline_l124_18;
  reg                 decode_to_execute_BYPASSABLE_EXECUTE_STAGE;
  wire                when_Pipeline_l124_19;
  reg                 decode_to_execute_BYPASSABLE_MEMORY_STAGE;
  wire                when_Pipeline_l124_20;
  reg                 execute_to_memory_BYPASSABLE_MEMORY_STAGE;
  wire                when_Pipeline_l124_21;
  reg                 decode_to_execute_MEMORY_STORE;
  wire                when_Pipeline_l124_22;
  reg                 execute_to_memory_MEMORY_STORE;
  wire                when_Pipeline_l124_23;
  reg                 decode_to_execute_IS_CSR;
  wire                when_Pipeline_l124_24;
  reg        [0:0]    decode_to_execute_ENV_CTRL;
  wire                when_Pipeline_l124_25;
  reg        [0:0]    execute_to_memory_ENV_CTRL;
  wire                when_Pipeline_l124_26;
  reg        [0:0]    memory_to_writeBack_ENV_CTRL;
  wire                when_Pipeline_l124_27;
  reg        [1:0]    decode_to_execute_ALU_CTRL;
  wire                when_Pipeline_l124_28;
  reg                 decode_to_execute_SRC_LESS_UNSIGNED;
  wire                when_Pipeline_l124_29;
  reg        [1:0]    decode_to_execute_ALU_BITWISE_CTRL;
  wire                when_Pipeline_l124_30;
  reg        [1:0]    decode_to_execute_SHIFT_CTRL;
  wire                when_Pipeline_l124_31;
  reg        [1:0]    decode_to_execute_BRANCH_CTRL;
  wire                when_Pipeline_l124_32;
  reg        [31:0]   decode_to_execute_RS1;
  wire                when_Pipeline_l124_33;
  reg        [31:0]   decode_to_execute_RS2;
  wire                when_Pipeline_l124_34;
  reg                 decode_to_execute_SRC2_FORCE_ZERO;
  wire                when_Pipeline_l124_35;
  reg        [31:0]   decode_to_execute_SRC1;
  wire                when_Pipeline_l124_36;
  reg        [31:0]   decode_to_execute_SRC2;
  wire                when_Pipeline_l124_37;
  reg                 decode_to_execute_DO_EBREAK;
  wire                when_Pipeline_l124_38;
  reg        [1:0]    execute_to_memory_MEMORY_ADDRESS_LOW;
  wire                when_Pipeline_l124_39;
  reg        [1:0]    memory_to_writeBack_MEMORY_ADDRESS_LOW;
  wire                when_Pipeline_l124_40;
  reg        [31:0]   execute_to_memory_REGFILE_WRITE_DATA;
  wire                when_Pipeline_l124_41;
  reg        [31:0]   memory_to_writeBack_REGFILE_WRITE_DATA;
  wire                when_Pipeline_l124_42;
  reg                 execute_to_memory_BRANCH_DO;
  wire                when_Pipeline_l124_43;
  reg        [31:0]   execute_to_memory_BRANCH_CALC;
  wire                when_Pipeline_l124_44;
  reg        [31:0]   memory_to_writeBack_MEMORY_READ_DATA;
  wire                when_Pipeline_l151;
  wire                when_Pipeline_l154;
  wire                when_Pipeline_l151_1;
  wire                when_Pipeline_l154_1;
  wire                when_Pipeline_l151_2;
  wire                when_Pipeline_l154_2;
  reg        [2:0]    switch_Fetcher_l365;
  wire                when_Fetcher_l381;
  wire                when_Fetcher_l401;
  wire                when_CsrPlugin_l1589;
  reg                 execute_CsrPlugin_csr_768;
  wire                when_CsrPlugin_l1589_1;
  reg                 execute_CsrPlugin_csr_836;
  wire                when_CsrPlugin_l1589_2;
  reg                 execute_CsrPlugin_csr_772;
  wire                when_CsrPlugin_l1589_3;
  reg                 execute_CsrPlugin_csr_834;
  wire       [1:0]    switch_CsrPlugin_l980;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_1;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_2;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_3;
  reg                 when_CsrPlugin_l1625;
  wire                when_CsrPlugin_l1623;
  wire                when_CsrPlugin_l1631;
  `ifndef SYNTHESIS
  reg [31:0] decode_BRANCH_CTRL_string;
  reg [31:0] _zz_decode_BRANCH_CTRL_string;
  reg [31:0] _zz_decode_to_execute_BRANCH_CTRL_string;
  reg [31:0] _zz_decode_to_execute_BRANCH_CTRL_1_string;
  reg [71:0] decode_SHIFT_CTRL_string;
  reg [71:0] _zz_decode_SHIFT_CTRL_string;
  reg [71:0] _zz_decode_to_execute_SHIFT_CTRL_string;
  reg [71:0] _zz_decode_to_execute_SHIFT_CTRL_1_string;
  reg [39:0] decode_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_decode_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_decode_to_execute_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_decode_to_execute_ALU_BITWISE_CTRL_1_string;
  reg [63:0] decode_ALU_CTRL_string;
  reg [63:0] _zz_decode_ALU_CTRL_string;
  reg [63:0] _zz_decode_to_execute_ALU_CTRL_string;
  reg [63:0] _zz_decode_to_execute_ALU_CTRL_1_string;
  reg [31:0] _zz_memory_to_writeBack_ENV_CTRL_string;
  reg [31:0] _zz_memory_to_writeBack_ENV_CTRL_1_string;
  reg [31:0] _zz_execute_to_memory_ENV_CTRL_string;
  reg [31:0] _zz_execute_to_memory_ENV_CTRL_1_string;
  reg [31:0] decode_ENV_CTRL_string;
  reg [31:0] _zz_decode_ENV_CTRL_string;
  reg [31:0] _zz_decode_to_execute_ENV_CTRL_string;
  reg [31:0] _zz_decode_to_execute_ENV_CTRL_1_string;
  reg [31:0] execute_BRANCH_CTRL_string;
  reg [31:0] _zz_execute_BRANCH_CTRL_string;
  reg [71:0] execute_SHIFT_CTRL_string;
  reg [71:0] _zz_execute_SHIFT_CTRL_string;
  reg [23:0] decode_SRC2_CTRL_string;
  reg [23:0] _zz_decode_SRC2_CTRL_string;
  reg [95:0] decode_SRC1_CTRL_string;
  reg [95:0] _zz_decode_SRC1_CTRL_string;
  reg [63:0] execute_ALU_CTRL_string;
  reg [63:0] _zz_execute_ALU_CTRL_string;
  reg [39:0] execute_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_execute_ALU_BITWISE_CTRL_string;
  reg [31:0] _zz_decode_BRANCH_CTRL_1_string;
  reg [71:0] _zz_decode_SHIFT_CTRL_1_string;
  reg [39:0] _zz_decode_ALU_BITWISE_CTRL_1_string;
  reg [63:0] _zz_decode_ALU_CTRL_1_string;
  reg [31:0] _zz_decode_ENV_CTRL_1_string;
  reg [23:0] _zz_decode_SRC2_CTRL_1_string;
  reg [95:0] _zz_decode_SRC1_CTRL_1_string;
  reg [31:0] memory_ENV_CTRL_string;
  reg [31:0] _zz_memory_ENV_CTRL_string;
  reg [31:0] execute_ENV_CTRL_string;
  reg [31:0] _zz_execute_ENV_CTRL_string;
  reg [31:0] writeBack_ENV_CTRL_string;
  reg [31:0] _zz_writeBack_ENV_CTRL_string;
  reg [95:0] _zz_decode_SRC1_CTRL_2_string;
  reg [23:0] _zz_decode_SRC2_CTRL_2_string;
  reg [31:0] _zz_decode_ENV_CTRL_2_string;
  reg [63:0] _zz_decode_ALU_CTRL_2_string;
  reg [39:0] _zz_decode_ALU_BITWISE_CTRL_2_string;
  reg [71:0] _zz_decode_SHIFT_CTRL_2_string;
  reg [31:0] _zz_decode_BRANCH_CTRL_2_string;
  reg [31:0] decode_to_execute_ENV_CTRL_string;
  reg [31:0] execute_to_memory_ENV_CTRL_string;
  reg [31:0] memory_to_writeBack_ENV_CTRL_string;
  reg [63:0] decode_to_execute_ALU_CTRL_string;
  reg [39:0] decode_to_execute_ALU_BITWISE_CTRL_string;
  reg [71:0] decode_to_execute_SHIFT_CTRL_string;
  reg [31:0] decode_to_execute_BRANCH_CTRL_string;
  `endif

  reg [31:0] RegFilePlugin_regFile [0:31] /* verilator public */ ;

  assign _zz_IBusSimplePlugin_jump_pcLoad_payload_1 = (_zz_IBusSimplePlugin_jump_pcLoad_payload & (~ _zz_IBusSimplePlugin_jump_pcLoad_payload_2));
  assign _zz_IBusSimplePlugin_jump_pcLoad_payload_2 = (_zz_IBusSimplePlugin_jump_pcLoad_payload - 2'b01);
  assign _zz_IBusSimplePlugin_fetchPc_pc_1 = {IBusSimplePlugin_fetchPc_inc,2'b00};
  assign _zz_IBusSimplePlugin_fetchPc_pc = {29'd0, _zz_IBusSimplePlugin_fetchPc_pc_1};
  assign _zz_IBusSimplePlugin_pending_next = (IBusSimplePlugin_pending_value + _zz_IBusSimplePlugin_pending_next_1);
  assign _zz_IBusSimplePlugin_pending_next_2 = IBusSimplePlugin_pending_inc;
  assign _zz_IBusSimplePlugin_pending_next_1 = {2'd0, _zz_IBusSimplePlugin_pending_next_2};
  assign _zz_IBusSimplePlugin_pending_next_4 = IBusSimplePlugin_pending_dec;
  assign _zz_IBusSimplePlugin_pending_next_3 = {2'd0, _zz_IBusSimplePlugin_pending_next_4};
  assign _zz_IBusSimplePlugin_rspJoin_rspBuffer_discardCounter_1 = (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_valid && (IBusSimplePlugin_rspJoin_rspBuffer_discardCounter != 3'b000));
  assign _zz_IBusSimplePlugin_rspJoin_rspBuffer_discardCounter = {2'd0, _zz_IBusSimplePlugin_rspJoin_rspBuffer_discardCounter_1};
  assign _zz__zz_execute_REGFILE_WRITE_DATA = execute_SRC_LESS;
  assign _zz__zz_decode_SRC1 = 3'b100;
  assign _zz__zz_decode_SRC1_1 = decode_INSTRUCTION[19 : 15];
  assign _zz__zz_decode_SRC2_2 = {decode_INSTRUCTION[31 : 25],decode_INSTRUCTION[11 : 7]};
  assign _zz_execute_SrcPlugin_addSub = ($signed(_zz_execute_SrcPlugin_addSub_1) + $signed(_zz_execute_SrcPlugin_addSub_4));
  assign _zz_execute_SrcPlugin_addSub_1 = ($signed(_zz_execute_SrcPlugin_addSub_2) + $signed(_zz_execute_SrcPlugin_addSub_3));
  assign _zz_execute_SrcPlugin_addSub_2 = execute_SRC1;
  assign _zz_execute_SrcPlugin_addSub_3 = (execute_SRC_USE_SUB_LESS ? (~ execute_SRC2) : execute_SRC2);
  assign _zz_execute_SrcPlugin_addSub_4 = (execute_SRC_USE_SUB_LESS ? _zz_execute_SrcPlugin_addSub_5 : _zz_execute_SrcPlugin_addSub_6);
  assign _zz_execute_SrcPlugin_addSub_5 = 32'h00000001;
  assign _zz_execute_SrcPlugin_addSub_6 = 32'h0;
  assign _zz__zz_decode_RS2_3 = (_zz__zz_decode_RS2_3_1 >>> 1);
  assign _zz__zz_decode_RS2_3_1 = {((execute_SHIFT_CTRL == ShiftCtrlEnum_SRA_1) && execute_LightShifterPlugin_shiftInput[31]),execute_LightShifterPlugin_shiftInput};
  assign _zz__zz_execute_BranchPlugin_branch_src2 = {{{execute_INSTRUCTION[31],execute_INSTRUCTION[19 : 12]},execute_INSTRUCTION[20]},execute_INSTRUCTION[30 : 21]};
  assign _zz__zz_execute_BranchPlugin_branch_src2_4 = {{{execute_INSTRUCTION[31],execute_INSTRUCTION[7]},execute_INSTRUCTION[30 : 25]},execute_INSTRUCTION[11 : 8]};
  assign _zz_decode_RegFilePlugin_rs1Data = 1'b1;
  assign _zz_decode_RegFilePlugin_rs2Data = 1'b1;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED = (decode_INSTRUCTION & 32'h0000001c);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_1 = 32'h00000004;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_2 = (decode_INSTRUCTION & 32'h00000058);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_3 = 32'h00000040;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_4 = ((decode_INSTRUCTION & 32'h00007054) == 32'h00005010);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_5 = {((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_6) == 32'h40001010),((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_7) == 32'h00001010)};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_8 = (|{(_zz__zz_decode_SRC_LESS_UNSIGNED_9 == _zz__zz_decode_SRC_LESS_UNSIGNED_10),(_zz__zz_decode_SRC_LESS_UNSIGNED_11 == _zz__zz_decode_SRC_LESS_UNSIGNED_12)});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_13 = (|(_zz__zz_decode_SRC_LESS_UNSIGNED_14 == _zz__zz_decode_SRC_LESS_UNSIGNED_15));
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_16 = {(|_zz__zz_decode_SRC_LESS_UNSIGNED_17),{(|_zz__zz_decode_SRC_LESS_UNSIGNED_18),{_zz__zz_decode_SRC_LESS_UNSIGNED_21,{_zz__zz_decode_SRC_LESS_UNSIGNED_26,_zz__zz_decode_SRC_LESS_UNSIGNED_27}}}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_6 = 32'h40003054;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_7 = 32'h00007054;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_9 = (decode_INSTRUCTION & 32'h00000064);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_10 = 32'h00000024;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_11 = (decode_INSTRUCTION & 32'h00003054);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_12 = 32'h00001010;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_14 = (decode_INSTRUCTION & 32'h00001000);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_15 = 32'h00001000;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_17 = ((decode_INSTRUCTION & 32'h00003000) == 32'h00002000);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_18 = {((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_19) == 32'h00002000),((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_20) == 32'h00001000)};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_21 = (|{(_zz__zz_decode_SRC_LESS_UNSIGNED_22 == _zz__zz_decode_SRC_LESS_UNSIGNED_23),(_zz__zz_decode_SRC_LESS_UNSIGNED_24 == _zz__zz_decode_SRC_LESS_UNSIGNED_25)});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_26 = (|_zz_decode_SRC_LESS_UNSIGNED_2);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_27 = {(|_zz__zz_decode_SRC_LESS_UNSIGNED_28),{(|_zz__zz_decode_SRC_LESS_UNSIGNED_29),{_zz__zz_decode_SRC_LESS_UNSIGNED_32,{_zz__zz_decode_SRC_LESS_UNSIGNED_37,_zz__zz_decode_SRC_LESS_UNSIGNED_42}}}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_19 = 32'h00002010;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_20 = 32'h00005000;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_22 = (decode_INSTRUCTION & 32'h00006004);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_23 = 32'h00006000;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_24 = (decode_INSTRUCTION & 32'h00005004);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_25 = 32'h00004000;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_28 = ((decode_INSTRUCTION & 32'h00103050) == 32'h00000050);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_29 = {((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_30) == 32'h00001050),((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_31) == 32'h00002050)};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_32 = (|{(_zz__zz_decode_SRC_LESS_UNSIGNED_33 == _zz__zz_decode_SRC_LESS_UNSIGNED_34),(_zz__zz_decode_SRC_LESS_UNSIGNED_35 == _zz__zz_decode_SRC_LESS_UNSIGNED_36)});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_37 = (|{_zz__zz_decode_SRC_LESS_UNSIGNED_38,{_zz__zz_decode_SRC_LESS_UNSIGNED_39,_zz__zz_decode_SRC_LESS_UNSIGNED_40}});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_42 = {(|_zz__zz_decode_SRC_LESS_UNSIGNED_43),{(|_zz__zz_decode_SRC_LESS_UNSIGNED_44),{_zz__zz_decode_SRC_LESS_UNSIGNED_46,{_zz__zz_decode_SRC_LESS_UNSIGNED_47,_zz__zz_decode_SRC_LESS_UNSIGNED_59}}}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_30 = 32'h00001050;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_31 = 32'h00002050;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_33 = (decode_INSTRUCTION & 32'h00000034);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_34 = 32'h00000020;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_35 = (decode_INSTRUCTION & 32'h00000064);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_36 = 32'h00000020;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_38 = ((decode_INSTRUCTION & 32'h00000050) == 32'h00000040);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_39 = _zz_decode_SRC_LESS_UNSIGNED_3;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_40 = ((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_41) == 32'h00000040);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_43 = ((decode_INSTRUCTION & 32'h00000020) == 32'h00000020);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_44 = ((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_45) == 32'h00000010);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_46 = (|_zz_decode_SRC_LESS_UNSIGNED_5);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_47 = (|{_zz__zz_decode_SRC_LESS_UNSIGNED_48,_zz__zz_decode_SRC_LESS_UNSIGNED_49});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_59 = {(|_zz__zz_decode_SRC_LESS_UNSIGNED_60),{_zz__zz_decode_SRC_LESS_UNSIGNED_63,{_zz__zz_decode_SRC_LESS_UNSIGNED_65,_zz__zz_decode_SRC_LESS_UNSIGNED_70}}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_41 = 32'h00103040;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_45 = 32'h00000010;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_48 = _zz_decode_SRC_LESS_UNSIGNED_6;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_49 = {(_zz__zz_decode_SRC_LESS_UNSIGNED_50 == _zz__zz_decode_SRC_LESS_UNSIGNED_51),{_zz__zz_decode_SRC_LESS_UNSIGNED_52,{_zz__zz_decode_SRC_LESS_UNSIGNED_53,_zz__zz_decode_SRC_LESS_UNSIGNED_54}}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_60 = {_zz_decode_SRC_LESS_UNSIGNED_4,(_zz__zz_decode_SRC_LESS_UNSIGNED_61 == _zz__zz_decode_SRC_LESS_UNSIGNED_62)};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_63 = (|{_zz_decode_SRC_LESS_UNSIGNED_4,_zz__zz_decode_SRC_LESS_UNSIGNED_64});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_65 = (|{_zz__zz_decode_SRC_LESS_UNSIGNED_66,_zz__zz_decode_SRC_LESS_UNSIGNED_68});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_70 = {(|_zz__zz_decode_SRC_LESS_UNSIGNED_71),{_zz__zz_decode_SRC_LESS_UNSIGNED_73,{_zz__zz_decode_SRC_LESS_UNSIGNED_77,_zz__zz_decode_SRC_LESS_UNSIGNED_80}}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_50 = (decode_INSTRUCTION & 32'h00001010);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_51 = 32'h00001010;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_52 = ((decode_INSTRUCTION & 32'h00002010) == 32'h00002010);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_53 = _zz_decode_SRC_LESS_UNSIGNED_5;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_54 = {(_zz__zz_decode_SRC_LESS_UNSIGNED_55 == _zz__zz_decode_SRC_LESS_UNSIGNED_56),(_zz__zz_decode_SRC_LESS_UNSIGNED_57 == _zz__zz_decode_SRC_LESS_UNSIGNED_58)};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_61 = (decode_INSTRUCTION & 32'h00000070);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_62 = 32'h00000020;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_64 = ((decode_INSTRUCTION & 32'h00000020) == 32'h0);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_66 = ((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_67) == 32'h0);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_68 = {_zz_decode_SRC_LESS_UNSIGNED_3,{_zz_decode_SRC_LESS_UNSIGNED_2,_zz__zz_decode_SRC_LESS_UNSIGNED_69}};
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_71 = ((decode_INSTRUCTION & _zz__zz_decode_SRC_LESS_UNSIGNED_72) == 32'h0);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_73 = (|{_zz__zz_decode_SRC_LESS_UNSIGNED_74,{_zz__zz_decode_SRC_LESS_UNSIGNED_75,_zz__zz_decode_SRC_LESS_UNSIGNED_76}});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_77 = (|{_zz__zz_decode_SRC_LESS_UNSIGNED_78,_zz__zz_decode_SRC_LESS_UNSIGNED_79});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_80 = (|{_zz__zz_decode_SRC_LESS_UNSIGNED_81,_zz__zz_decode_SRC_LESS_UNSIGNED_82});
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_55 = (decode_INSTRUCTION & 32'h0000000c);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_56 = 32'h00000004;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_57 = (decode_INSTRUCTION & 32'h00000028);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_58 = 32'h0;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_67 = 32'h00000044;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_69 = ((decode_INSTRUCTION & 32'h00005004) == 32'h00001000);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_72 = 32'h00000058;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_74 = ((decode_INSTRUCTION & 32'h00000044) == 32'h00000040);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_75 = ((decode_INSTRUCTION & 32'h00002014) == 32'h00002010);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_76 = ((decode_INSTRUCTION & 32'h40004034) == 32'h40000030);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_78 = ((decode_INSTRUCTION & 32'h00000014) == 32'h00000004);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_79 = _zz_decode_SRC_LESS_UNSIGNED_1;
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_81 = ((decode_INSTRUCTION & 32'h00000044) == 32'h00000004);
  assign _zz__zz_decode_SRC_LESS_UNSIGNED_82 = _zz_decode_SRC_LESS_UNSIGNED_1;
  always @(posedge io_mainClk) begin
    if(_zz_decode_RegFilePlugin_rs1Data) begin
      _zz_RegFilePlugin_regFile_port0 <= RegFilePlugin_regFile[decode_RegFilePlugin_regFileReadAddress1];
    end
  end

  always @(posedge io_mainClk) begin
    if(_zz_decode_RegFilePlugin_rs2Data) begin
      _zz_RegFilePlugin_regFile_port1 <= RegFilePlugin_regFile[decode_RegFilePlugin_regFileReadAddress2];
    end
  end

  always @(posedge io_mainClk) begin
    if(_zz_1) begin
      RegFilePlugin_regFile[lastStageRegFileWrite_payload_address] <= lastStageRegFileWrite_payload_data;
    end
  end

  StreamFifoLowLatency IBusSimplePlugin_rspJoin_rspBuffer_c (
    .io_push_valid         (iBus_rsp_toStream_valid                                       ), //i
    .io_push_ready         (IBusSimplePlugin_rspJoin_rspBuffer_c_io_push_ready            ), //o
    .io_push_payload_error (iBus_rsp_toStream_payload_error                               ), //i
    .io_push_payload_inst  (iBus_rsp_toStream_payload_inst[31:0]                          ), //i
    .io_pop_valid          (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_valid             ), //o
    .io_pop_ready          (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_ready             ), //i
    .io_pop_payload_error  (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_payload_error     ), //o
    .io_pop_payload_inst   (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_payload_inst[31:0]), //o
    .io_flush              (1'b0                                                          ), //i
    .io_occupancy          (IBusSimplePlugin_rspJoin_rspBuffer_c_io_occupancy             ), //o
    .io_mainClk            (io_mainClk                                                    ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset                                         )  //i
  );
  `ifndef SYNTHESIS
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
    case(_zz_decode_BRANCH_CTRL)
      BranchCtrlEnum_INC : _zz_decode_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_BRANCH_CTRL_string = "JALR";
      default : _zz_decode_BRANCH_CTRL_string = "????";
    endcase
  end
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
    case(_zz_memory_to_writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_memory_to_writeBack_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_memory_to_writeBack_ENV_CTRL_string = "XRET";
      default : _zz_memory_to_writeBack_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_memory_to_writeBack_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_memory_to_writeBack_ENV_CTRL_1_string = "NONE";
      EnvCtrlEnum_XRET : _zz_memory_to_writeBack_ENV_CTRL_1_string = "XRET";
      default : _zz_memory_to_writeBack_ENV_CTRL_1_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_to_memory_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_execute_to_memory_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_execute_to_memory_ENV_CTRL_string = "XRET";
      default : _zz_execute_to_memory_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_to_memory_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_execute_to_memory_ENV_CTRL_1_string = "NONE";
      EnvCtrlEnum_XRET : _zz_execute_to_memory_ENV_CTRL_1_string = "XRET";
      default : _zz_execute_to_memory_ENV_CTRL_1_string = "????";
    endcase
  end
  always @(*) begin
    case(decode_ENV_CTRL)
      EnvCtrlEnum_NONE : decode_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : decode_ENV_CTRL_string = "XRET";
      default : decode_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_decode_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_decode_ENV_CTRL_string = "XRET";
      default : _zz_decode_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_decode_to_execute_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_decode_to_execute_ENV_CTRL_string = "XRET";
      default : _zz_decode_to_execute_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_decode_to_execute_ENV_CTRL_1_string = "NONE";
      EnvCtrlEnum_XRET : _zz_decode_to_execute_ENV_CTRL_1_string = "XRET";
      default : _zz_decode_to_execute_ENV_CTRL_1_string = "????";
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
    case(_zz_decode_BRANCH_CTRL_1)
      BranchCtrlEnum_INC : _zz_decode_BRANCH_CTRL_1_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_BRANCH_CTRL_1_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_BRANCH_CTRL_1_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_BRANCH_CTRL_1_string = "JALR";
      default : _zz_decode_BRANCH_CTRL_1_string = "????";
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
    case(_zz_decode_ALU_CTRL_1)
      AluCtrlEnum_ADD_SUB : _zz_decode_ALU_CTRL_1_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_decode_ALU_CTRL_1_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_decode_ALU_CTRL_1_string = "BITWISE ";
      default : _zz_decode_ALU_CTRL_1_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_decode_ENV_CTRL_1_string = "NONE";
      EnvCtrlEnum_XRET : _zz_decode_ENV_CTRL_1_string = "XRET";
      default : _zz_decode_ENV_CTRL_1_string = "????";
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
    case(_zz_decode_SRC1_CTRL_1)
      Src1CtrlEnum_RS : _zz_decode_SRC1_CTRL_1_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_decode_SRC1_CTRL_1_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_decode_SRC1_CTRL_1_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_decode_SRC1_CTRL_1_string = "URS1        ";
      default : _zz_decode_SRC1_CTRL_1_string = "????????????";
    endcase
  end
  always @(*) begin
    case(memory_ENV_CTRL)
      EnvCtrlEnum_NONE : memory_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : memory_ENV_CTRL_string = "XRET";
      default : memory_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_memory_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_memory_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_memory_ENV_CTRL_string = "XRET";
      default : _zz_memory_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(execute_ENV_CTRL)
      EnvCtrlEnum_NONE : execute_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : execute_ENV_CTRL_string = "XRET";
      default : execute_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_execute_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_execute_ENV_CTRL_string = "XRET";
      default : _zz_execute_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : writeBack_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : writeBack_ENV_CTRL_string = "XRET";
      default : writeBack_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_writeBack_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : _zz_writeBack_ENV_CTRL_string = "XRET";
      default : _zz_writeBack_ENV_CTRL_string = "????";
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
    case(_zz_decode_SRC2_CTRL_2)
      Src2CtrlEnum_RS : _zz_decode_SRC2_CTRL_2_string = "RS ";
      Src2CtrlEnum_IMI : _zz_decode_SRC2_CTRL_2_string = "IMI";
      Src2CtrlEnum_IMS : _zz_decode_SRC2_CTRL_2_string = "IMS";
      Src2CtrlEnum_PC : _zz_decode_SRC2_CTRL_2_string = "PC ";
      default : _zz_decode_SRC2_CTRL_2_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ENV_CTRL_2)
      EnvCtrlEnum_NONE : _zz_decode_ENV_CTRL_2_string = "NONE";
      EnvCtrlEnum_XRET : _zz_decode_ENV_CTRL_2_string = "XRET";
      default : _zz_decode_ENV_CTRL_2_string = "????";
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
    case(_zz_decode_BRANCH_CTRL_2)
      BranchCtrlEnum_INC : _zz_decode_BRANCH_CTRL_2_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_BRANCH_CTRL_2_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_BRANCH_CTRL_2_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_BRANCH_CTRL_2_string = "JALR";
      default : _zz_decode_BRANCH_CTRL_2_string = "????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_ENV_CTRL)
      EnvCtrlEnum_NONE : decode_to_execute_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : decode_to_execute_ENV_CTRL_string = "XRET";
      default : decode_to_execute_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(execute_to_memory_ENV_CTRL)
      EnvCtrlEnum_NONE : execute_to_memory_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : execute_to_memory_ENV_CTRL_string = "XRET";
      default : execute_to_memory_ENV_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(memory_to_writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : memory_to_writeBack_ENV_CTRL_string = "NONE";
      EnvCtrlEnum_XRET : memory_to_writeBack_ENV_CTRL_string = "XRET";
      default : memory_to_writeBack_ENV_CTRL_string = "????";
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
    case(decode_to_execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : decode_to_execute_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : decode_to_execute_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : decode_to_execute_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : decode_to_execute_BRANCH_CTRL_string = "JALR";
      default : decode_to_execute_BRANCH_CTRL_string = "????";
    endcase
  end
  `endif

  assign memory_MEMORY_READ_DATA = dBus_rsp_data; // @[Stage.scala 30:13]
  assign execute_BRANCH_CALC = {execute_BranchPlugin_branchAdder[31 : 1],1'b0}; // @[Stage.scala 30:13]
  assign execute_BRANCH_DO = _zz_execute_BRANCH_DO_1; // @[Stage.scala 30:13]
  assign writeBack_REGFILE_WRITE_DATA = memory_to_writeBack_REGFILE_WRITE_DATA; // @[Stage.scala 30:13]
  assign execute_REGFILE_WRITE_DATA = _zz_execute_REGFILE_WRITE_DATA; // @[Stage.scala 30:13]
  assign memory_MEMORY_ADDRESS_LOW = execute_to_memory_MEMORY_ADDRESS_LOW; // @[Stage.scala 30:13]
  assign execute_MEMORY_ADDRESS_LOW = dBus_cmd_payload_address[1 : 0]; // @[Stage.scala 30:13]
  assign decode_DO_EBREAK = (((! DebugPlugin_haltIt) && (decode_IS_EBREAK || 1'b0)) && DebugPlugin_allowEBreak); // @[Stage.scala 30:13]
  assign decode_SRC2 = _zz_decode_SRC2_4; // @[Stage.scala 30:13]
  assign decode_SRC1 = _zz_decode_SRC1; // @[Stage.scala 30:13]
  assign decode_SRC2_FORCE_ZERO = (decode_SRC_ADD_ZERO && (! decode_SRC_USE_SUB_LESS)); // @[Stage.scala 30:13]
  assign decode_BRANCH_CTRL = _zz_decode_BRANCH_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_BRANCH_CTRL = _zz_decode_to_execute_BRANCH_CTRL_1; // @[Stage.scala 39:14]
  assign decode_SHIFT_CTRL = _zz_decode_SHIFT_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_SHIFT_CTRL = _zz_decode_to_execute_SHIFT_CTRL_1; // @[Stage.scala 39:14]
  assign decode_ALU_BITWISE_CTRL = _zz_decode_ALU_BITWISE_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_ALU_BITWISE_CTRL = _zz_decode_to_execute_ALU_BITWISE_CTRL_1; // @[Stage.scala 39:14]
  assign decode_SRC_LESS_UNSIGNED = _zz_decode_SRC_LESS_UNSIGNED[17]; // @[Stage.scala 30:13]
  assign decode_ALU_CTRL = _zz_decode_ALU_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_ALU_CTRL = _zz_decode_to_execute_ALU_CTRL_1; // @[Stage.scala 39:14]
  assign _zz_memory_to_writeBack_ENV_CTRL = _zz_memory_to_writeBack_ENV_CTRL_1; // @[Stage.scala 39:14]
  assign _zz_execute_to_memory_ENV_CTRL = _zz_execute_to_memory_ENV_CTRL_1; // @[Stage.scala 39:14]
  assign decode_ENV_CTRL = _zz_decode_ENV_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_ENV_CTRL = _zz_decode_to_execute_ENV_CTRL_1; // @[Stage.scala 39:14]
  assign decode_IS_CSR = _zz_decode_SRC_LESS_UNSIGNED[13]; // @[Stage.scala 30:13]
  assign decode_MEMORY_STORE = _zz_decode_SRC_LESS_UNSIGNED[10]; // @[Stage.scala 30:13]
  assign execute_BYPASSABLE_MEMORY_STAGE = decode_to_execute_BYPASSABLE_MEMORY_STAGE; // @[Stage.scala 30:13]
  assign decode_BYPASSABLE_MEMORY_STAGE = _zz_decode_SRC_LESS_UNSIGNED[9]; // @[Stage.scala 30:13]
  assign decode_BYPASSABLE_EXECUTE_STAGE = _zz_decode_SRC_LESS_UNSIGNED[8]; // @[Stage.scala 30:13]
  assign decode_MEMORY_ENABLE = _zz_decode_SRC_LESS_UNSIGNED[3]; // @[Stage.scala 30:13]
  assign decode_CSR_READ_OPCODE = (decode_INSTRUCTION[13 : 7] != 7'h20); // @[Stage.scala 30:13]
  assign decode_CSR_WRITE_OPCODE = (! (((decode_INSTRUCTION[14 : 13] == 2'b01) && (decode_INSTRUCTION[19 : 15] == 5'h0)) || ((decode_INSTRUCTION[14 : 13] == 2'b11) && (decode_INSTRUCTION[19 : 15] == 5'h0)))); // @[Stage.scala 30:13]
  assign writeBack_FORMAL_PC_NEXT = memory_to_writeBack_FORMAL_PC_NEXT; // @[Stage.scala 30:13]
  assign memory_FORMAL_PC_NEXT = execute_to_memory_FORMAL_PC_NEXT; // @[Stage.scala 30:13]
  assign execute_FORMAL_PC_NEXT = decode_to_execute_FORMAL_PC_NEXT; // @[Stage.scala 30:13]
  assign decode_FORMAL_PC_NEXT = (decode_PC + 32'h00000004); // @[Stage.scala 30:13]
  assign memory_PC = execute_to_memory_PC; // @[Stage.scala 30:13]
  assign execute_DO_EBREAK = decode_to_execute_DO_EBREAK; // @[Stage.scala 30:13]
  assign decode_IS_EBREAK = _zz_decode_SRC_LESS_UNSIGNED[25]; // @[Stage.scala 30:13]
  assign memory_BRANCH_CALC = execute_to_memory_BRANCH_CALC; // @[Stage.scala 30:13]
  assign memory_BRANCH_DO = execute_to_memory_BRANCH_DO; // @[Stage.scala 30:13]
  assign execute_PC = decode_to_execute_PC; // @[Stage.scala 30:13]
  assign execute_RS1 = decode_to_execute_RS1; // @[Stage.scala 30:13]
  assign execute_BRANCH_CTRL = _zz_execute_BRANCH_CTRL; // @[Stage.scala 30:13]
  assign decode_RS2_USE = _zz_decode_SRC_LESS_UNSIGNED[12]; // @[Stage.scala 30:13]
  assign decode_RS1_USE = _zz_decode_SRC_LESS_UNSIGNED[4]; // @[Stage.scala 30:13]
  assign execute_REGFILE_WRITE_VALID = decode_to_execute_REGFILE_WRITE_VALID; // @[Stage.scala 30:13]
  assign execute_BYPASSABLE_EXECUTE_STAGE = decode_to_execute_BYPASSABLE_EXECUTE_STAGE; // @[Stage.scala 30:13]
  assign _zz_decode_RS2 = memory_REGFILE_WRITE_DATA; // @[Stage.scala 39:14]
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
          decode_RS2 = _zz_decode_RS2; // @[HazardSimplePlugin.scala 52:38]
        end
      end
    end
    if(when_HazardSimplePlugin_l45_2) begin
      if(execute_BYPASSABLE_EXECUTE_STAGE) begin
        if(when_HazardSimplePlugin_l51_2) begin
          decode_RS2 = _zz_decode_RS2_1; // @[HazardSimplePlugin.scala 52:38]
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
          decode_RS1 = _zz_decode_RS2; // @[HazardSimplePlugin.scala 49:38]
        end
      end
    end
    if(when_HazardSimplePlugin_l45_2) begin
      if(execute_BYPASSABLE_EXECUTE_STAGE) begin
        if(when_HazardSimplePlugin_l48_2) begin
          decode_RS1 = _zz_decode_RS2_1; // @[HazardSimplePlugin.scala 49:38]
        end
      end
    end
  end

  assign memory_REGFILE_WRITE_DATA = execute_to_memory_REGFILE_WRITE_DATA; // @[Stage.scala 30:13]
  assign execute_SHIFT_CTRL = _zz_execute_SHIFT_CTRL; // @[Stage.scala 30:13]
  assign execute_SRC_LESS_UNSIGNED = decode_to_execute_SRC_LESS_UNSIGNED; // @[Stage.scala 30:13]
  assign execute_SRC2_FORCE_ZERO = decode_to_execute_SRC2_FORCE_ZERO; // @[Stage.scala 30:13]
  assign execute_SRC_USE_SUB_LESS = decode_to_execute_SRC_USE_SUB_LESS; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_PC = decode_PC; // @[Stage.scala 39:14]
  assign _zz_decode_to_execute_RS2 = decode_RS2; // @[Stage.scala 39:14]
  assign decode_SRC2_CTRL = _zz_decode_SRC2_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_RS1 = decode_RS1; // @[Stage.scala 39:14]
  assign decode_SRC1_CTRL = _zz_decode_SRC1_CTRL; // @[Stage.scala 30:13]
  assign decode_SRC_USE_SUB_LESS = _zz_decode_SRC_LESS_UNSIGNED[2]; // @[Stage.scala 30:13]
  assign decode_SRC_ADD_ZERO = _zz_decode_SRC_LESS_UNSIGNED[20]; // @[Stage.scala 30:13]
  assign execute_SRC_ADD_SUB = execute_SrcPlugin_addSub; // @[Stage.scala 30:13]
  assign execute_SRC_LESS = execute_SrcPlugin_less; // @[Stage.scala 30:13]
  assign execute_ALU_CTRL = _zz_execute_ALU_CTRL; // @[Stage.scala 30:13]
  assign execute_SRC2 = decode_to_execute_SRC2; // @[Stage.scala 30:13]
  assign execute_ALU_BITWISE_CTRL = _zz_execute_ALU_BITWISE_CTRL; // @[Stage.scala 30:13]
  assign _zz_lastStageRegFileWrite_payload_address = writeBack_INSTRUCTION; // @[Stage.scala 39:14]
  assign _zz_lastStageRegFileWrite_valid = writeBack_REGFILE_WRITE_VALID; // @[Stage.scala 39:14]
  always @(*) begin
    _zz_1 = 1'b0; // @[when.scala 47:16]
    if(lastStageRegFileWrite_valid) begin
      _zz_1 = 1'b1; // @[when.scala 52:10]
    end
  end

  assign decode_INSTRUCTION_ANTICIPATED = (decode_arbitration_isStuck ? decode_INSTRUCTION : IBusSimplePlugin_iBusRsp_output_payload_rsp_inst); // @[Stage.scala 30:13]
  always @(*) begin
    decode_REGFILE_WRITE_VALID = _zz_decode_SRC_LESS_UNSIGNED[7]; // @[Stage.scala 30:13]
    if(when_RegFilePlugin_l63) begin
      decode_REGFILE_WRITE_VALID = 1'b0; // @[RegFilePlugin.scala 64:41]
    end
  end

  always @(*) begin
    _zz_decode_RS2_1 = execute_REGFILE_WRITE_DATA; // @[Stage.scala 39:14]
    if(when_CsrPlugin_l1507) begin
      _zz_decode_RS2_1 = CsrPlugin_csrMapping_readDataSignal; // @[CsrPlugin.scala 1508:59]
    end
    if(when_ShiftPlugins_l169) begin
      _zz_decode_RS2_1 = _zz_decode_RS2_3; // @[ShiftPlugins.scala 170:36]
    end
  end

  assign execute_SRC1 = decode_to_execute_SRC1; // @[Stage.scala 30:13]
  assign execute_CSR_READ_OPCODE = decode_to_execute_CSR_READ_OPCODE; // @[Stage.scala 30:13]
  assign execute_CSR_WRITE_OPCODE = decode_to_execute_CSR_WRITE_OPCODE; // @[Stage.scala 30:13]
  assign execute_IS_CSR = decode_to_execute_IS_CSR; // @[Stage.scala 30:13]
  assign memory_ENV_CTRL = _zz_memory_ENV_CTRL; // @[Stage.scala 30:13]
  assign execute_ENV_CTRL = _zz_execute_ENV_CTRL; // @[Stage.scala 30:13]
  assign writeBack_ENV_CTRL = _zz_writeBack_ENV_CTRL; // @[Stage.scala 30:13]
  always @(*) begin
    _zz_decode_RS2_2 = writeBack_REGFILE_WRITE_DATA; // @[Stage.scala 39:14]
    if(when_DBusSimplePlugin_l558) begin
      _zz_decode_RS2_2 = writeBack_DBusSimplePlugin_rspFormated; // @[DBusSimplePlugin.scala 559:36]
    end
  end

  assign writeBack_MEMORY_ENABLE = memory_to_writeBack_MEMORY_ENABLE; // @[Stage.scala 30:13]
  assign writeBack_MEMORY_ADDRESS_LOW = memory_to_writeBack_MEMORY_ADDRESS_LOW; // @[Stage.scala 30:13]
  assign writeBack_MEMORY_READ_DATA = memory_to_writeBack_MEMORY_READ_DATA; // @[Stage.scala 30:13]
  assign memory_MEMORY_STORE = execute_to_memory_MEMORY_STORE; // @[Stage.scala 30:13]
  assign memory_MEMORY_ENABLE = execute_to_memory_MEMORY_ENABLE; // @[Stage.scala 30:13]
  assign execute_SRC_ADD = execute_SrcPlugin_addSub; // @[Stage.scala 30:13]
  assign execute_RS2 = decode_to_execute_RS2; // @[Stage.scala 30:13]
  assign execute_INSTRUCTION = decode_to_execute_INSTRUCTION; // @[Stage.scala 30:13]
  assign execute_MEMORY_STORE = decode_to_execute_MEMORY_STORE; // @[Stage.scala 30:13]
  assign execute_MEMORY_ENABLE = decode_to_execute_MEMORY_ENABLE; // @[Stage.scala 30:13]
  assign execute_ALIGNEMENT_FAULT = 1'b0; // @[Stage.scala 30:13]
  always @(*) begin
    _zz_memory_to_writeBack_FORMAL_PC_NEXT = memory_FORMAL_PC_NEXT; // @[Stage.scala 39:14]
    if(BranchPlugin_jumpInterface_valid) begin
      _zz_memory_to_writeBack_FORMAL_PC_NEXT = BranchPlugin_jumpInterface_payload; // @[Fetcher.scala 437:47]
    end
  end

  assign decode_PC = IBusSimplePlugin_injector_decodeInput_payload_pc; // @[Stage.scala 30:13]
  assign decode_INSTRUCTION = IBusSimplePlugin_injector_decodeInput_payload_rsp_inst; // @[Stage.scala 30:13]
  assign writeBack_PC = memory_to_writeBack_PC; // @[Stage.scala 30:13]
  assign writeBack_INSTRUCTION = memory_to_writeBack_INSTRUCTION; // @[Stage.scala 30:13]
  always @(*) begin
    decode_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
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
    if(CsrPlugin_pipelineLiberator_active) begin
      decode_arbitration_haltByOther = 1'b1; // @[CsrPlugin.scala 1253:42]
    end
    if(when_CsrPlugin_l1447) begin
      decode_arbitration_haltByOther = 1'b1; // @[CsrPlugin.scala 1447:38]
    end
    if(when_HazardSimplePlugin_l113) begin
      decode_arbitration_haltByOther = 1'b1; // @[HazardSimplePlugin.scala 114:43]
    end
  end

  always @(*) begin
    decode_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(decode_arbitration_isFlushed) begin
      decode_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  assign decode_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
  assign decode_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
  always @(*) begin
    execute_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
    if(when_DBusSimplePlugin_l428) begin
      execute_arbitration_haltItself = 1'b1; // @[DBusSimplePlugin.scala 429:32]
    end
    if(when_CsrPlugin_l1511) begin
      if(execute_CsrPlugin_blockedBySideEffects) begin
        execute_arbitration_haltItself = 1'b1; // @[CsrPlugin.scala 1512:34]
      end
    end
    if(when_ShiftPlugins_l169) begin
      if(when_ShiftPlugins_l184) begin
        execute_arbitration_haltItself = 1'b1; // @[ShiftPlugins.scala 185:34]
      end
    end
  end

  always @(*) begin
    execute_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
    if(when_DebugPlugin_l308) begin
      execute_arbitration_haltByOther = 1'b1; // @[DebugPlugin.scala 309:41]
    end
  end

  always @(*) begin
    execute_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
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
    if(when_DebugPlugin_l308) begin
      if(when_DebugPlugin_l311) begin
        execute_arbitration_flushNext = 1'b1; // @[DebugPlugin.scala 314:41]
      end
    end
  end

  always @(*) begin
    memory_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
    if(when_DBusSimplePlugin_l482) begin
      memory_arbitration_haltItself = 1'b1; // @[DBusSimplePlugin.scala 482:30]
    end
  end

  assign memory_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
  always @(*) begin
    memory_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(memory_arbitration_isFlushed) begin
      memory_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  assign memory_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
  always @(*) begin
    memory_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
    if(BranchPlugin_jumpInterface_valid) begin
      memory_arbitration_flushNext = 1'b1; // @[BranchPlugin.scala 215:29]
    end
  end

  assign writeBack_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
  assign writeBack_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
  always @(*) begin
    writeBack_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(writeBack_arbitration_isFlushed) begin
      writeBack_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  assign writeBack_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
  always @(*) begin
    writeBack_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
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
    IBusSimplePlugin_fetcherHalt = 1'b0; // @[Fetcher.scala 67:19]
    if(when_CsrPlugin_l1310) begin
      IBusSimplePlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_CsrPlugin_l1376) begin
      IBusSimplePlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_DebugPlugin_l308) begin
      if(when_DebugPlugin_l311) begin
        IBusSimplePlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
      end
    end
    if(DebugPlugin_haltIt) begin
      IBusSimplePlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_DebugPlugin_l324) begin
      IBusSimplePlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
  end

  assign IBusSimplePlugin_forceNoDecodeCond = 1'b0; // @[Fetcher.scala 68:25]
  always @(*) begin
    IBusSimplePlugin_incomingInstruction = 1'b0; // @[Fetcher.scala 69:27]
    if(when_Fetcher_l243) begin
      IBusSimplePlugin_incomingInstruction = 1'b1; // @[Fetcher.scala 243:27]
    end
    if(IBusSimplePlugin_injector_decodeInput_valid) begin
      IBusSimplePlugin_incomingInstruction = 1'b1; // @[Fetcher.scala 317:29]
    end
  end

  assign CsrPlugin_csrMapping_allowCsrSignal = 1'b0; // @[CsrPlugin.scala 358:24]
  assign CsrPlugin_csrMapping_readDataSignal = CsrPlugin_csrMapping_readDataInit; // @[CsrPlugin.scala 361:18]
  assign CsrPlugin_inWfi = 1'b0; // @[CsrPlugin.scala 552:13]
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

  assign CsrPlugin_xretAwayFromMachine = 1'b0; // @[CsrPlugin.scala 637:27]
  always @(*) begin
    BranchPlugin_inDebugNoFetchFlag = 1'b0; // @[BranchPlugin.scala 155:26]
    if(DebugPlugin_godmode) begin
      BranchPlugin_inDebugNoFetchFlag = 1'b1; // @[BranchPlugin.scala 90:60]
    end
  end

  assign IBusSimplePlugin_externalFlush = ({writeBack_arbitration_flushNext,{memory_arbitration_flushNext,{execute_arbitration_flushNext,decode_arbitration_flushNext}}} != 4'b0000); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_jump_pcLoad_valid = ({BranchPlugin_jumpInterface_valid,CsrPlugin_jumpInterface_valid} != 2'b00); // @[Fetcher.scala 116:20]
  assign _zz_IBusSimplePlugin_jump_pcLoad_payload = {BranchPlugin_jumpInterface_valid,CsrPlugin_jumpInterface_valid}; // @[BaseType.scala 318:22]
  assign IBusSimplePlugin_jump_pcLoad_payload = (_zz_IBusSimplePlugin_jump_pcLoad_payload_1[0] ? CsrPlugin_jumpInterface_payload : BranchPlugin_jumpInterface_payload); // @[Fetcher.scala 117:22]
  always @(*) begin
    IBusSimplePlugin_fetchPc_correction = 1'b0; // @[Fetcher.scala 129:24]
    if(IBusSimplePlugin_jump_pcLoad_valid) begin
      IBusSimplePlugin_fetchPc_correction = 1'b1; // @[Fetcher.scala 156:20]
    end
  end

  assign IBusSimplePlugin_fetchPc_output_fire = (IBusSimplePlugin_fetchPc_output_valid && IBusSimplePlugin_fetchPc_output_ready); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_fetchPc_corrected = (IBusSimplePlugin_fetchPc_correction || IBusSimplePlugin_fetchPc_correctionReg); // @[BaseType.scala 305:24]
  always @(*) begin
    IBusSimplePlugin_fetchPc_pcRegPropagate = 1'b0; // @[Fetcher.scala 132:28]
    if(IBusSimplePlugin_iBusRsp_stages_1_input_ready) begin
      IBusSimplePlugin_fetchPc_pcRegPropagate = 1'b1; // @[Fetcher.scala 235:34]
    end
  end

  assign when_Fetcher_l134 = (IBusSimplePlugin_fetchPc_correction || IBusSimplePlugin_fetchPc_pcRegPropagate); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_fetchPc_output_fire_1 = (IBusSimplePlugin_fetchPc_output_valid && IBusSimplePlugin_fetchPc_output_ready); // @[BaseType.scala 305:24]
  assign when_Fetcher_l134_1 = ((! IBusSimplePlugin_fetchPc_output_valid) && IBusSimplePlugin_fetchPc_output_ready); // @[BaseType.scala 305:24]
  always @(*) begin
    IBusSimplePlugin_fetchPc_pc = (IBusSimplePlugin_fetchPc_pcReg + _zz_IBusSimplePlugin_fetchPc_pc); // @[BaseType.scala 299:24]
    if(IBusSimplePlugin_jump_pcLoad_valid) begin
      IBusSimplePlugin_fetchPc_pc = IBusSimplePlugin_jump_pcLoad_payload; // @[Fetcher.scala 157:12]
    end
    IBusSimplePlugin_fetchPc_pc[0] = 1'b0; // @[Fetcher.scala 165:13]
    IBusSimplePlugin_fetchPc_pc[1] = 1'b0; // @[Fetcher.scala 166:32]
  end

  always @(*) begin
    IBusSimplePlugin_fetchPc_flushed = 1'b0; // @[Fetcher.scala 138:21]
    if(IBusSimplePlugin_jump_pcLoad_valid) begin
      IBusSimplePlugin_fetchPc_flushed = 1'b1; // @[Fetcher.scala 158:17]
    end
  end

  assign when_Fetcher_l161 = (IBusSimplePlugin_fetchPc_booted && ((IBusSimplePlugin_fetchPc_output_ready || IBusSimplePlugin_fetchPc_correction) || IBusSimplePlugin_fetchPc_pcRegPropagate)); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_fetchPc_output_valid = ((! IBusSimplePlugin_fetcherHalt) && IBusSimplePlugin_fetchPc_booted); // @[Fetcher.scala 168:20]
  assign IBusSimplePlugin_fetchPc_output_payload = IBusSimplePlugin_fetchPc_pc; // @[Fetcher.scala 169:22]
  assign IBusSimplePlugin_iBusRsp_redoFetch = 1'b0; // @[Fetcher.scala 210:23]
  assign IBusSimplePlugin_iBusRsp_stages_0_input_valid = IBusSimplePlugin_fetchPc_output_valid; // @[Stream.scala 294:16]
  assign IBusSimplePlugin_fetchPc_output_ready = IBusSimplePlugin_iBusRsp_stages_0_input_ready; // @[Stream.scala 295:16]
  assign IBusSimplePlugin_iBusRsp_stages_0_input_payload = IBusSimplePlugin_fetchPc_output_payload; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_iBusRsp_stages_0_halt = 1'b0; // @[Fetcher.scala 219:16]
  assign _zz_IBusSimplePlugin_iBusRsp_stages_0_input_ready = (! IBusSimplePlugin_iBusRsp_stages_0_halt); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_iBusRsp_stages_0_input_ready = (IBusSimplePlugin_iBusRsp_stages_0_output_ready && _zz_IBusSimplePlugin_iBusRsp_stages_0_input_ready); // @[Stream.scala 427:16]
  assign IBusSimplePlugin_iBusRsp_stages_0_output_valid = (IBusSimplePlugin_iBusRsp_stages_0_input_valid && _zz_IBusSimplePlugin_iBusRsp_stages_0_input_ready); // @[Stream.scala 294:16]
  assign IBusSimplePlugin_iBusRsp_stages_0_output_payload = IBusSimplePlugin_iBusRsp_stages_0_input_payload; // @[Stream.scala 296:18]
  always @(*) begin
    IBusSimplePlugin_iBusRsp_stages_1_halt = 1'b0; // @[Fetcher.scala 219:16]
    if(when_IBusSimplePlugin_l305) begin
      IBusSimplePlugin_iBusRsp_stages_1_halt = 1'b1; // @[IBusSimplePlugin.scala 305:20]
    end
  end

  assign _zz_IBusSimplePlugin_iBusRsp_stages_1_input_ready = (! IBusSimplePlugin_iBusRsp_stages_1_halt); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_iBusRsp_stages_1_input_ready = (IBusSimplePlugin_iBusRsp_stages_1_output_ready && _zz_IBusSimplePlugin_iBusRsp_stages_1_input_ready); // @[Stream.scala 427:16]
  assign IBusSimplePlugin_iBusRsp_stages_1_output_valid = (IBusSimplePlugin_iBusRsp_stages_1_input_valid && _zz_IBusSimplePlugin_iBusRsp_stages_1_input_ready); // @[Stream.scala 294:16]
  assign IBusSimplePlugin_iBusRsp_stages_1_output_payload = IBusSimplePlugin_iBusRsp_stages_1_input_payload; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_iBusRsp_stages_2_halt = 1'b0; // @[Fetcher.scala 219:16]
  assign _zz_IBusSimplePlugin_iBusRsp_stages_2_input_ready = (! IBusSimplePlugin_iBusRsp_stages_2_halt); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_iBusRsp_stages_2_input_ready = (IBusSimplePlugin_iBusRsp_stages_2_output_ready && _zz_IBusSimplePlugin_iBusRsp_stages_2_input_ready); // @[Stream.scala 427:16]
  assign IBusSimplePlugin_iBusRsp_stages_2_output_valid = (IBusSimplePlugin_iBusRsp_stages_2_input_valid && _zz_IBusSimplePlugin_iBusRsp_stages_2_input_ready); // @[Stream.scala 294:16]
  assign IBusSimplePlugin_iBusRsp_stages_2_output_payload = IBusSimplePlugin_iBusRsp_stages_2_input_payload; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_iBusRsp_flush = (IBusSimplePlugin_externalFlush || IBusSimplePlugin_iBusRsp_redoFetch); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_iBusRsp_stages_0_output_ready = _zz_IBusSimplePlugin_iBusRsp_stages_0_output_ready; // @[Stream.scala 304:16]
  assign _zz_IBusSimplePlugin_iBusRsp_stages_0_output_ready = ((1'b0 && (! _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid)) || IBusSimplePlugin_iBusRsp_stages_1_input_ready); // @[Misc.scala 148:20]
  assign _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid = _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid_1; // @[Misc.scala 158:17]
  assign IBusSimplePlugin_iBusRsp_stages_1_input_valid = _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid; // @[Stream.scala 303:16]
  assign IBusSimplePlugin_iBusRsp_stages_1_input_payload = IBusSimplePlugin_fetchPc_pcReg; // @[Fetcher.scala 234:31]
  assign IBusSimplePlugin_iBusRsp_stages_1_output_ready = ((1'b0 && (! IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid)) || IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_ready); // @[Misc.scala 148:20]
  assign IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid = _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid; // @[Misc.scala 158:17]
  assign IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_payload = _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_payload; // @[Misc.scala 159:19]
  assign IBusSimplePlugin_iBusRsp_stages_2_input_valid = IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid; // @[Stream.scala 294:16]
  assign IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_ready = IBusSimplePlugin_iBusRsp_stages_2_input_ready; // @[Stream.scala 295:16]
  assign IBusSimplePlugin_iBusRsp_stages_2_input_payload = IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_payload; // @[Stream.scala 296:18]
  always @(*) begin
    IBusSimplePlugin_iBusRsp_readyForError = 1'b1; // @[Fetcher.scala 241:27]
    if(IBusSimplePlugin_injector_decodeInput_valid) begin
      IBusSimplePlugin_iBusRsp_readyForError = 1'b0; // @[Fetcher.scala 316:40]
    end
    if(when_Fetcher_l323) begin
      IBusSimplePlugin_iBusRsp_readyForError = 1'b0; // @[Fetcher.scala 323:55]
    end
  end

  assign when_Fetcher_l243 = (IBusSimplePlugin_iBusRsp_stages_1_input_valid || IBusSimplePlugin_iBusRsp_stages_2_input_valid); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_iBusRsp_output_ready = ((1'b0 && (! IBusSimplePlugin_injector_decodeInput_valid)) || IBusSimplePlugin_injector_decodeInput_ready); // @[Misc.scala 148:20]
  assign IBusSimplePlugin_injector_decodeInput_valid = _zz_IBusSimplePlugin_injector_decodeInput_valid; // @[Misc.scala 158:17]
  assign IBusSimplePlugin_injector_decodeInput_payload_pc = _zz_IBusSimplePlugin_injector_decodeInput_payload_pc; // @[Misc.scala 159:19]
  assign IBusSimplePlugin_injector_decodeInput_payload_rsp_error = _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_error; // @[Misc.scala 159:19]
  assign IBusSimplePlugin_injector_decodeInput_payload_rsp_inst = _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_inst; // @[Misc.scala 159:19]
  assign IBusSimplePlugin_injector_decodeInput_payload_isRvc = _zz_IBusSimplePlugin_injector_decodeInput_payload_isRvc; // @[Misc.scala 159:19]
  assign when_Fetcher_l323 = (! IBusSimplePlugin_pcValids_0); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332 = (! (! IBusSimplePlugin_iBusRsp_stages_1_input_ready)); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_1 = (! (! IBusSimplePlugin_iBusRsp_stages_2_input_ready)); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_2 = (! (! IBusSimplePlugin_injector_decodeInput_ready)); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_3 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_4 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_5 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_pcValids_0 = IBusSimplePlugin_injector_nextPcCalc_valids_2; // @[Fetcher.scala 348:18]
  assign IBusSimplePlugin_pcValids_1 = IBusSimplePlugin_injector_nextPcCalc_valids_3; // @[Fetcher.scala 348:18]
  assign IBusSimplePlugin_pcValids_2 = IBusSimplePlugin_injector_nextPcCalc_valids_4; // @[Fetcher.scala 348:18]
  assign IBusSimplePlugin_pcValids_3 = IBusSimplePlugin_injector_nextPcCalc_valids_5; // @[Fetcher.scala 348:18]
  assign IBusSimplePlugin_injector_decodeInput_ready = (! decode_arbitration_isStuck); // @[Fetcher.scala 351:25]
  always @(*) begin
    decode_arbitration_isValid = IBusSimplePlugin_injector_decodeInput_valid; // @[Fetcher.scala 352:34]
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
    if(IBusSimplePlugin_forceNoDecodeCond) begin
      decode_arbitration_isValid = 1'b0; // @[Fetcher.scala 415:36]
    end
  end

  assign iBus_cmd_valid = IBusSimplePlugin_cmd_valid; // @[Stream.scala 294:16]
  assign IBusSimplePlugin_cmd_ready = iBus_cmd_ready; // @[Stream.scala 295:16]
  assign iBus_cmd_payload_pc = IBusSimplePlugin_cmd_payload_pc; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_pending_next = (_zz_IBusSimplePlugin_pending_next - _zz_IBusSimplePlugin_pending_next_3); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_cmdFork_canEmit = (IBusSimplePlugin_iBusRsp_stages_1_output_ready && (IBusSimplePlugin_pending_value != 3'b111)); // @[BaseType.scala 305:24]
  assign when_IBusSimplePlugin_l305 = (IBusSimplePlugin_iBusRsp_stages_1_input_valid && ((! IBusSimplePlugin_cmdFork_canEmit) || (! IBusSimplePlugin_cmd_ready))); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_cmd_valid = (IBusSimplePlugin_iBusRsp_stages_1_input_valid && IBusSimplePlugin_cmdFork_canEmit); // @[IBusSimplePlugin.scala 306:19]
  assign IBusSimplePlugin_cmd_fire = (IBusSimplePlugin_cmd_valid && IBusSimplePlugin_cmd_ready); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_pending_inc = IBusSimplePlugin_cmd_fire; // @[IBusSimplePlugin.scala 307:21]
  assign IBusSimplePlugin_cmd_payload_pc = {IBusSimplePlugin_iBusRsp_stages_1_input_payload[31 : 2],2'b00}; // @[IBusSimplePlugin.scala 347:16]
  assign iBus_rsp_toStream_valid = iBus_rsp_valid; // @[Flow.scala 72:15]
  assign iBus_rsp_toStream_payload_error = iBus_rsp_payload_error; // @[Flow.scala 73:17]
  assign iBus_rsp_toStream_payload_inst = iBus_rsp_payload_inst; // @[Flow.scala 73:17]
  assign iBus_rsp_toStream_ready = IBusSimplePlugin_rspJoin_rspBuffer_c_io_push_ready; // @[Stream.scala 295:16]
  assign IBusSimplePlugin_rspJoin_rspBuffer_flush = ((IBusSimplePlugin_rspJoin_rspBuffer_discardCounter != 3'b000) || IBusSimplePlugin_iBusRsp_flush); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_rspJoin_rspBuffer_output_valid = (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_valid && (IBusSimplePlugin_rspJoin_rspBuffer_discardCounter == 3'b000)); // @[IBusSimplePlugin.scala 366:24]
  assign IBusSimplePlugin_rspJoin_rspBuffer_output_payload_error = IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_payload_error; // @[IBusSimplePlugin.scala 367:26]
  assign IBusSimplePlugin_rspJoin_rspBuffer_output_payload_inst = IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_payload_inst; // @[IBusSimplePlugin.scala 367:26]
  assign IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_ready = (IBusSimplePlugin_rspJoin_rspBuffer_output_ready || IBusSimplePlugin_rspJoin_rspBuffer_flush); // @[IBusSimplePlugin.scala 368:26]
  assign system_cpu_IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_fire = (IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_valid && IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_ready); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_pending_dec = system_cpu_IBusSimplePlugin_rspJoin_rspBuffer_c_io_pop_fire; // @[IBusSimplePlugin.scala 370:23]
  assign IBusSimplePlugin_rspJoin_fetchRsp_pc = IBusSimplePlugin_iBusRsp_stages_2_output_payload; // @[IBusSimplePlugin.scala 374:21]
  always @(*) begin
    IBusSimplePlugin_rspJoin_fetchRsp_rsp_error = IBusSimplePlugin_rspJoin_rspBuffer_output_payload_error; // @[IBusSimplePlugin.scala 375:22]
    if(when_IBusSimplePlugin_l376) begin
      IBusSimplePlugin_rspJoin_fetchRsp_rsp_error = 1'b0; // @[IBusSimplePlugin.scala 376:37]
    end
  end

  assign IBusSimplePlugin_rspJoin_fetchRsp_rsp_inst = IBusSimplePlugin_rspJoin_rspBuffer_output_payload_inst; // @[IBusSimplePlugin.scala 375:22]
  assign when_IBusSimplePlugin_l376 = (! IBusSimplePlugin_rspJoin_rspBuffer_output_valid); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_rspJoin_exceptionDetected = 1'b0; // @[IBusSimplePlugin.scala 384:33]
  assign IBusSimplePlugin_rspJoin_join_valid = (IBusSimplePlugin_iBusRsp_stages_2_output_valid && IBusSimplePlugin_rspJoin_rspBuffer_output_valid); // @[IBusSimplePlugin.scala 385:20]
  assign IBusSimplePlugin_rspJoin_join_payload_pc = IBusSimplePlugin_rspJoin_fetchRsp_pc; // @[IBusSimplePlugin.scala 386:22]
  assign IBusSimplePlugin_rspJoin_join_payload_rsp_error = IBusSimplePlugin_rspJoin_fetchRsp_rsp_error; // @[IBusSimplePlugin.scala 386:22]
  assign IBusSimplePlugin_rspJoin_join_payload_rsp_inst = IBusSimplePlugin_rspJoin_fetchRsp_rsp_inst; // @[IBusSimplePlugin.scala 386:22]
  assign IBusSimplePlugin_rspJoin_join_payload_isRvc = IBusSimplePlugin_rspJoin_fetchRsp_isRvc; // @[IBusSimplePlugin.scala 386:22]
  assign IBusSimplePlugin_rspJoin_join_fire = (IBusSimplePlugin_rspJoin_join_valid && IBusSimplePlugin_rspJoin_join_ready); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_iBusRsp_stages_2_output_ready = (IBusSimplePlugin_iBusRsp_stages_2_output_valid ? IBusSimplePlugin_rspJoin_join_fire : IBusSimplePlugin_rspJoin_join_ready); // @[IBusSimplePlugin.scala 387:34]
  assign IBusSimplePlugin_rspJoin_join_fire_1 = (IBusSimplePlugin_rspJoin_join_valid && IBusSimplePlugin_rspJoin_join_ready); // @[BaseType.scala 305:24]
  assign IBusSimplePlugin_rspJoin_rspBuffer_output_ready = IBusSimplePlugin_rspJoin_join_fire_1; // @[IBusSimplePlugin.scala 388:32]
  assign _zz_IBusSimplePlugin_iBusRsp_output_valid = (! IBusSimplePlugin_rspJoin_exceptionDetected); // @[BaseType.scala 299:24]
  assign IBusSimplePlugin_rspJoin_join_ready = (IBusSimplePlugin_iBusRsp_output_ready && _zz_IBusSimplePlugin_iBusRsp_output_valid); // @[Stream.scala 427:16]
  assign IBusSimplePlugin_iBusRsp_output_valid = (IBusSimplePlugin_rspJoin_join_valid && _zz_IBusSimplePlugin_iBusRsp_output_valid); // @[Stream.scala 294:16]
  assign IBusSimplePlugin_iBusRsp_output_payload_pc = IBusSimplePlugin_rspJoin_join_payload_pc; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_iBusRsp_output_payload_rsp_error = IBusSimplePlugin_rspJoin_join_payload_rsp_error; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_iBusRsp_output_payload_rsp_inst = IBusSimplePlugin_rspJoin_join_payload_rsp_inst; // @[Stream.scala 296:18]
  assign IBusSimplePlugin_iBusRsp_output_payload_isRvc = IBusSimplePlugin_rspJoin_join_payload_isRvc; // @[Stream.scala 296:18]
  assign _zz_dBus_cmd_valid = 1'b0; // @[DBusSimplePlugin.scala 404:127]
  always @(*) begin
    execute_DBusSimplePlugin_skipCmd = 1'b0; // @[DBusSimplePlugin.scala 417:21]
    if(execute_ALIGNEMENT_FAULT) begin
      execute_DBusSimplePlugin_skipCmd = 1'b1; // @[DBusSimplePlugin.scala 418:15]
    end
  end

  assign dBus_cmd_valid = (((((execute_arbitration_isValid && execute_MEMORY_ENABLE) && (! execute_arbitration_isStuckByOthers)) && (! execute_arbitration_isFlushed)) && (! execute_DBusSimplePlugin_skipCmd)) && (! _zz_dBus_cmd_valid)); // @[DBusSimplePlugin.scala 420:22]
  assign dBus_cmd_payload_wr = execute_MEMORY_STORE; // @[DBusSimplePlugin.scala 421:19]
  assign dBus_cmd_payload_size = execute_INSTRUCTION[13 : 12]; // @[DBusSimplePlugin.scala 422:21]
  always @(*) begin
    case(dBus_cmd_payload_size)
      2'b00 : begin
        _zz_dBus_cmd_payload_data = {{{execute_RS2[7 : 0],execute_RS2[7 : 0]},execute_RS2[7 : 0]},execute_RS2[7 : 0]}; // @[Misc.scala 239:22]
      end
      2'b01 : begin
        _zz_dBus_cmd_payload_data = {execute_RS2[15 : 0],execute_RS2[15 : 0]}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_dBus_cmd_payload_data = execute_RS2[31 : 0]; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign dBus_cmd_payload_data = _zz_dBus_cmd_payload_data; // @[DBusSimplePlugin.scala 423:29]
  assign when_DBusSimplePlugin_l428 = ((((execute_arbitration_isValid && execute_MEMORY_ENABLE) && (! dBus_cmd_ready)) && (! execute_DBusSimplePlugin_skipCmd)) && (! _zz_dBus_cmd_valid)); // @[BaseType.scala 305:24]
  always @(*) begin
    case(dBus_cmd_payload_size)
      2'b00 : begin
        _zz_execute_DBusSimplePlugin_formalMask = 4'b0001; // @[Misc.scala 239:22]
      end
      2'b01 : begin
        _zz_execute_DBusSimplePlugin_formalMask = 4'b0011; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_DBusSimplePlugin_formalMask = 4'b1111; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign execute_DBusSimplePlugin_formalMask = (_zz_execute_DBusSimplePlugin_formalMask <<< dBus_cmd_payload_address[1 : 0]); // @[BaseType.scala 299:24]
  assign dBus_cmd_payload_address = execute_SRC_ADD; // @[DBusSimplePlugin.scala 458:26]
  assign when_DBusSimplePlugin_l482 = (((memory_arbitration_isValid && memory_MEMORY_ENABLE) && (! memory_MEMORY_STORE)) && ((! dBus_rsp_ready) || 1'b0)); // @[BaseType.scala 305:24]
  always @(*) begin
    writeBack_DBusSimplePlugin_rspShifted = writeBack_MEMORY_READ_DATA; // @[DBusSimplePlugin.scala 530:18]
    case(writeBack_MEMORY_ADDRESS_LOW)
      2'b01 : begin
        writeBack_DBusSimplePlugin_rspShifted[7 : 0] = writeBack_MEMORY_READ_DATA[15 : 8]; // @[DBusSimplePlugin.scala 539:40]
      end
      2'b10 : begin
        writeBack_DBusSimplePlugin_rspShifted[15 : 0] = writeBack_MEMORY_READ_DATA[31 : 16]; // @[DBusSimplePlugin.scala 540:41]
      end
      2'b11 : begin
        writeBack_DBusSimplePlugin_rspShifted[7 : 0] = writeBack_MEMORY_READ_DATA[31 : 24]; // @[DBusSimplePlugin.scala 541:40]
      end
      default : begin
      end
    endcase
  end

  assign switch_Misc_l226 = writeBack_INSTRUCTION[13 : 12]; // @[BaseType.scala 299:24]
  assign _zz_writeBack_DBusSimplePlugin_rspFormated = (writeBack_DBusSimplePlugin_rspShifted[7] && (! writeBack_INSTRUCTION[14])); // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[31] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[30] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[29] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[28] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[27] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[26] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[25] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[24] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[23] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[22] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[21] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[20] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[19] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[18] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[17] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[16] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[15] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[14] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[13] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[12] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[11] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[10] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[9] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[8] = _zz_writeBack_DBusSimplePlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_1[7 : 0] = writeBack_DBusSimplePlugin_rspShifted[7 : 0]; // @[Literal.scala 99:91]
  end

  assign _zz_writeBack_DBusSimplePlugin_rspFormated_2 = (writeBack_DBusSimplePlugin_rspShifted[15] && (! writeBack_INSTRUCTION[14])); // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[31] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[30] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[29] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[28] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[27] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[26] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[25] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[24] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[23] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[22] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[21] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[20] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[19] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[18] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[17] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[16] = _zz_writeBack_DBusSimplePlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusSimplePlugin_rspFormated_3[15 : 0] = writeBack_DBusSimplePlugin_rspShifted[15 : 0]; // @[Literal.scala 99:91]
  end

  always @(*) begin
    case(switch_Misc_l226)
      2'b00 : begin
        writeBack_DBusSimplePlugin_rspFormated = _zz_writeBack_DBusSimplePlugin_rspFormated_1; // @[Misc.scala 239:22]
      end
      2'b01 : begin
        writeBack_DBusSimplePlugin_rspFormated = _zz_writeBack_DBusSimplePlugin_rspFormated_3; // @[Misc.scala 239:22]
      end
      default : begin
        writeBack_DBusSimplePlugin_rspFormated = writeBack_DBusSimplePlugin_rspShifted; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign when_DBusSimplePlugin_l558 = (writeBack_arbitration_isValid && writeBack_MEMORY_ENABLE); // @[BaseType.scala 305:24]
  always @(*) begin
    CsrPlugin_privilege = 2'b11; // @[CsrPlugin.scala 680:15]
    if(CsrPlugin_forceMachineWire) begin
      CsrPlugin_privilege = 2'b11; // @[CsrPlugin.scala 682:40]
    end
  end

  assign CsrPlugin_misa_base = 2'b01;
  assign CsrPlugin_misa_extensions = 26'h0000042;
  assign CsrPlugin_mtvec_mode = 2'b00;
  assign CsrPlugin_mtvec_base = 30'h20000008;
  assign _zz_when_CsrPlugin_l1222 = (CsrPlugin_mip_MTIP && CsrPlugin_mie_MTIE); // @[BaseType.scala 305:24]
  assign _zz_when_CsrPlugin_l1222_1 = (CsrPlugin_mip_MSIP && CsrPlugin_mie_MSIE); // @[BaseType.scala 305:24]
  assign _zz_when_CsrPlugin_l1222_2 = (CsrPlugin_mip_MEIP && CsrPlugin_mie_MEIE); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1216 = (CsrPlugin_mstatus_MIE || (CsrPlugin_privilege < 2'b11)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1222 = ((_zz_when_CsrPlugin_l1222 && 1'b1) && (! 1'b0)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1222_1 = ((_zz_when_CsrPlugin_l1222_1 && 1'b1) && (! 1'b0)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1222_2 = ((_zz_when_CsrPlugin_l1222_2 && 1'b1) && (! 1'b0)); // @[BaseType.scala 305:24]
  assign CsrPlugin_exception = 1'b0; // @[CsrPlugin.scala 1243:115]
  assign CsrPlugin_lastStageWasWfi = 1'b0; // @[CsrPlugin.scala 1244:152]
  assign CsrPlugin_pipelineLiberator_active = ((CsrPlugin_interrupt_valid && CsrPlugin_allowInterrupts) && decode_arbitration_isValid); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1255 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1255_1 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1255_2 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1260 = ((! CsrPlugin_pipelineLiberator_active) || decode_arbitration_removeIt); // @[BaseType.scala 305:24]
  always @(*) begin
    CsrPlugin_pipelineLiberator_done = CsrPlugin_pipelineLiberator_pcValids_2; // @[Misc.scala 552:9]
    if(CsrPlugin_hadException) begin
      CsrPlugin_pipelineLiberator_done = 1'b0; // @[CsrPlugin.scala 1275:39]
    end
  end

  assign CsrPlugin_interruptJump = ((CsrPlugin_interrupt_valid && CsrPlugin_pipelineLiberator_done) && CsrPlugin_allowInterrupts); // @[CsrPlugin.scala 1271:21]
  assign CsrPlugin_targetPrivilege = CsrPlugin_interrupt_targetPrivilege; // @[Misc.scala 552:9]
  assign CsrPlugin_trapCause = CsrPlugin_interrupt_code; // @[Misc.scala 552:9]
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
  assign contextSwitching = CsrPlugin_jumpInterface_valid; // @[CsrPlugin.scala 1405:24]
  assign when_CsrPlugin_l1447 = (|{(writeBack_arbitration_isValid && (writeBack_ENV_CTRL == EnvCtrlEnum_XRET)),{(memory_arbitration_isValid && (memory_ENV_CTRL == EnvCtrlEnum_XRET)),(execute_arbitration_isValid && (execute_ENV_CTRL == EnvCtrlEnum_XRET))}}); // @[BaseType.scala 312:24]
  assign execute_CsrPlugin_blockedBySideEffects = ((|{writeBack_arbitration_isValid,memory_arbitration_isValid}) || 1'b0); // @[BaseType.scala 305:24]
  always @(*) begin
    execute_CsrPlugin_illegalAccess = 1'b1; // @[CsrPlugin.scala 1454:29]
    if(execute_CsrPlugin_csr_768) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_836) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_772) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_834) begin
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

  assign when_CsrPlugin_l1467 = (execute_arbitration_isValid && (execute_ENV_CTRL == EnvCtrlEnum_XRET)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1468 = (CsrPlugin_privilege < execute_INSTRUCTION[29 : 28]); // @[BaseType.scala 305:24]
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
  assign _zz_decode_SRC_LESS_UNSIGNED_1 = ((decode_INSTRUCTION & 32'h00004050) == 32'h00004050); // @[BaseType.scala 305:24]
  assign _zz_decode_SRC_LESS_UNSIGNED_2 = ((decode_INSTRUCTION & 32'h00006004) == 32'h00002000); // @[BaseType.scala 305:24]
  assign _zz_decode_SRC_LESS_UNSIGNED_3 = ((decode_INSTRUCTION & 32'h00000018) == 32'h0); // @[BaseType.scala 305:24]
  assign _zz_decode_SRC_LESS_UNSIGNED_4 = ((decode_INSTRUCTION & 32'h00000004) == 32'h00000004); // @[BaseType.scala 305:24]
  assign _zz_decode_SRC_LESS_UNSIGNED_5 = ((decode_INSTRUCTION & 32'h00000050) == 32'h00000010); // @[BaseType.scala 305:24]
  assign _zz_decode_SRC_LESS_UNSIGNED_6 = ((decode_INSTRUCTION & 32'h00000048) == 32'h00000048); // @[BaseType.scala 305:24]
  assign _zz_decode_SRC_LESS_UNSIGNED = {(|((decode_INSTRUCTION & 32'h10003050) == 32'h00000050)),{(|{_zz_decode_SRC_LESS_UNSIGNED_6,(_zz__zz_decode_SRC_LESS_UNSIGNED == _zz__zz_decode_SRC_LESS_UNSIGNED_1)}),{(|(_zz__zz_decode_SRC_LESS_UNSIGNED_2 == _zz__zz_decode_SRC_LESS_UNSIGNED_3)),{(|_zz__zz_decode_SRC_LESS_UNSIGNED_4),{(|_zz__zz_decode_SRC_LESS_UNSIGNED_5),{_zz__zz_decode_SRC_LESS_UNSIGNED_8,{_zz__zz_decode_SRC_LESS_UNSIGNED_13,_zz__zz_decode_SRC_LESS_UNSIGNED_16}}}}}}}; // @[DecoderSimplePlugin.scala 161:19]
  assign _zz_decode_SRC1_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[1 : 0]; // @[Enum.scala 186:17]
  assign _zz_decode_SRC1_CTRL_1 = _zz_decode_SRC1_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_SRC2_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[6 : 5]; // @[Enum.scala 186:17]
  assign _zz_decode_SRC2_CTRL_1 = _zz_decode_SRC2_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_ENV_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[14 : 14]; // @[Enum.scala 186:17]
  assign _zz_decode_ENV_CTRL_1 = _zz_decode_ENV_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_ALU_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[16 : 15]; // @[Enum.scala 186:17]
  assign _zz_decode_ALU_CTRL_1 = _zz_decode_ALU_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_ALU_BITWISE_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[19 : 18]; // @[Enum.scala 186:17]
  assign _zz_decode_ALU_BITWISE_CTRL_1 = _zz_decode_ALU_BITWISE_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_SHIFT_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[22 : 21]; // @[Enum.scala 186:17]
  assign _zz_decode_SHIFT_CTRL_1 = _zz_decode_SHIFT_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_BRANCH_CTRL_2 = _zz_decode_SRC_LESS_UNSIGNED[24 : 23]; // @[Enum.scala 186:17]
  assign _zz_decode_BRANCH_CTRL_1 = _zz_decode_BRANCH_CTRL_2; // @[Enum.scala 188:10]
  assign when_RegFilePlugin_l63 = (decode_INSTRUCTION[11 : 7] == 5'h0); // @[BaseType.scala 305:24]
  assign decode_RegFilePlugin_regFileReadAddress1 = decode_INSTRUCTION_ANTICIPATED[19 : 15]; // @[BaseType.scala 318:22]
  assign decode_RegFilePlugin_regFileReadAddress2 = decode_INSTRUCTION_ANTICIPATED[24 : 20]; // @[BaseType.scala 318:22]
  assign decode_RegFilePlugin_rs1Data = _zz_RegFilePlugin_regFile_port0; // @[Bits.scala 133:56]
  assign decode_RegFilePlugin_rs2Data = _zz_RegFilePlugin_regFile_port1; // @[Bits.scala 133:56]
  always @(*) begin
    lastStageRegFileWrite_valid = (_zz_lastStageRegFileWrite_valid && writeBack_arbitration_isFiring); // @[RegFilePlugin.scala 102:26]
    if(_zz_2) begin
      lastStageRegFileWrite_valid = 1'b1; // @[RegFilePlugin.scala 114:28]
    end
  end

  always @(*) begin
    lastStageRegFileWrite_payload_address = _zz_lastStageRegFileWrite_payload_address[11 : 7]; // @[RegFilePlugin.scala 103:28]
    if(_zz_2) begin
      lastStageRegFileWrite_payload_address = 5'h0; // @[RegFilePlugin.scala 116:32]
    end
  end

  always @(*) begin
    lastStageRegFileWrite_payload_data = _zz_decode_RS2_2; // @[RegFilePlugin.scala 104:25]
    if(_zz_2) begin
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
    case(decode_SRC1_CTRL)
      Src1CtrlEnum_RS : begin
        _zz_decode_SRC1 = _zz_decode_to_execute_RS1; // @[Misc.scala 239:22]
      end
      Src1CtrlEnum_PC_INCREMENT : begin
        _zz_decode_SRC1 = {29'd0, _zz__zz_decode_SRC1}; // @[Misc.scala 239:22]
      end
      Src1CtrlEnum_IMU : begin
        _zz_decode_SRC1 = {decode_INSTRUCTION[31 : 12],12'h0}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_decode_SRC1 = {27'd0, _zz__zz_decode_SRC1_1}; // @[Misc.scala 239:22]
      end
    endcase
  end

  assign _zz_decode_SRC2 = decode_INSTRUCTION[31]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_decode_SRC2_1[19] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[18] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[17] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[16] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[15] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[14] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[13] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[12] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[11] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[10] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[9] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[8] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[7] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[6] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[5] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[4] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[3] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[2] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[1] = _zz_decode_SRC2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_1[0] = _zz_decode_SRC2; // @[Literal.scala 87:17]
  end

  assign _zz_decode_SRC2_2 = _zz__zz_decode_SRC2_2[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_decode_SRC2_3[19] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[18] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[17] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[16] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[15] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[14] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[13] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[12] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[11] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[10] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[9] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[8] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[7] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[6] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[5] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[4] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[3] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[2] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[1] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
    _zz_decode_SRC2_3[0] = _zz_decode_SRC2_2; // @[Literal.scala 87:17]
  end

  always @(*) begin
    case(decode_SRC2_CTRL)
      Src2CtrlEnum_RS : begin
        _zz_decode_SRC2_4 = _zz_decode_to_execute_RS2; // @[Misc.scala 239:22]
      end
      Src2CtrlEnum_IMI : begin
        _zz_decode_SRC2_4 = {_zz_decode_SRC2_1,decode_INSTRUCTION[31 : 20]}; // @[Misc.scala 239:22]
      end
      Src2CtrlEnum_IMS : begin
        _zz_decode_SRC2_4 = {_zz_decode_SRC2_3,{decode_INSTRUCTION[31 : 25],decode_INSTRUCTION[11 : 7]}}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_decode_SRC2_4 = _zz_decode_to_execute_PC; // @[Misc.scala 239:22]
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
  assign execute_LightShifterPlugin_isShift = (execute_SHIFT_CTRL != ShiftCtrlEnum_DISABLE_1); // @[BaseType.scala 305:24]
  assign execute_LightShifterPlugin_amplitude = (execute_LightShifterPlugin_isActive ? execute_LightShifterPlugin_amplitudeReg : execute_SRC2[4 : 0]); // @[Expression.scala 1420:25]
  assign execute_LightShifterPlugin_shiftInput = (execute_LightShifterPlugin_isActive ? memory_REGFILE_WRITE_DATA : execute_SRC1); // @[Expression.scala 1420:25]
  assign execute_LightShifterPlugin_done = (execute_LightShifterPlugin_amplitude[4 : 1] == 4'b0000); // @[BaseType.scala 305:24]
  assign when_ShiftPlugins_l169 = ((execute_arbitration_isValid && execute_LightShifterPlugin_isShift) && (execute_SRC2[4 : 0] != 5'h0)); // @[BaseType.scala 305:24]
  always @(*) begin
    case(execute_SHIFT_CTRL)
      ShiftCtrlEnum_SLL_1 : begin
        _zz_decode_RS2_3 = (execute_LightShifterPlugin_shiftInput <<< 1); // @[Misc.scala 239:22]
      end
      default : begin
        _zz_decode_RS2_3 = _zz__zz_decode_RS2_3; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign when_ShiftPlugins_l175 = (! execute_arbitration_isStuckByOthers); // @[BaseType.scala 299:24]
  assign when_ShiftPlugins_l184 = (! execute_LightShifterPlugin_done); // @[BaseType.scala 299:24]
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
  assign execute_BranchPlugin_eq = (execute_SRC1 == execute_SRC2); // @[BaseType.scala 305:24]
  assign switch_Misc_l226_2 = execute_INSTRUCTION[14 : 12]; // @[BaseType.scala 299:24]
  always @(*) begin
    casez(switch_Misc_l226_2)
      3'b000 : begin
        _zz_execute_BRANCH_DO = execute_BranchPlugin_eq; // @[Misc.scala 239:22]
      end
      3'b001 : begin
        _zz_execute_BRANCH_DO = (! execute_BranchPlugin_eq); // @[Misc.scala 239:22]
      end
      3'b1?1 : begin
        _zz_execute_BRANCH_DO = (! execute_SRC_LESS); // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_BRANCH_DO = execute_SRC_LESS; // @[Misc.scala 235:22]
      end
    endcase
  end

  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : begin
        _zz_execute_BRANCH_DO_1 = 1'b0; // @[Misc.scala 239:22]
      end
      BranchCtrlEnum_JAL : begin
        _zz_execute_BRANCH_DO_1 = 1'b1; // @[Misc.scala 239:22]
      end
      BranchCtrlEnum_JALR : begin
        _zz_execute_BRANCH_DO_1 = 1'b1; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_BRANCH_DO_1 = _zz_execute_BRANCH_DO; // @[Misc.scala 239:22]
      end
    endcase
  end

  assign execute_BranchPlugin_branch_src1 = ((execute_BRANCH_CTRL == BranchCtrlEnum_JALR) ? execute_RS1 : execute_PC); // @[Expression.scala 1420:25]
  assign _zz_execute_BranchPlugin_branch_src2 = _zz__zz_execute_BranchPlugin_branch_src2[19]; // @[BaseType.scala 305:24]
  always @(*) begin
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

  assign _zz_execute_BranchPlugin_branch_src2_2 = execute_INSTRUCTION[31]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_branch_src2_3[19] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[18] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[17] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[16] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[15] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[14] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[13] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[12] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[11] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
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

  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_JAL : begin
        _zz_execute_BranchPlugin_branch_src2_6 = {{_zz_execute_BranchPlugin_branch_src2_1,{{{execute_INSTRUCTION[31],execute_INSTRUCTION[19 : 12]},execute_INSTRUCTION[20]},execute_INSTRUCTION[30 : 21]}},1'b0}; // @[Misc.scala 239:22]
      end
      BranchCtrlEnum_JALR : begin
        _zz_execute_BranchPlugin_branch_src2_6 = {_zz_execute_BranchPlugin_branch_src2_3,execute_INSTRUCTION[31 : 20]}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_BranchPlugin_branch_src2_6 = {{_zz_execute_BranchPlugin_branch_src2_5,{{{execute_INSTRUCTION[31],execute_INSTRUCTION[7]},execute_INSTRUCTION[30 : 25]},execute_INSTRUCTION[11 : 8]}},1'b0}; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign execute_BranchPlugin_branch_src2 = _zz_execute_BranchPlugin_branch_src2_6; // @[BaseType.scala 318:22]
  assign execute_BranchPlugin_branchAdder = (execute_BranchPlugin_branch_src1 + execute_BranchPlugin_branch_src2); // @[BaseType.scala 299:24]
  assign BranchPlugin_jumpInterface_valid = ((memory_arbitration_isValid && memory_BRANCH_DO) && (! 1'b0)); // @[BranchPlugin.scala 213:27]
  assign BranchPlugin_jumpInterface_payload = memory_BRANCH_CALC; // @[BranchPlugin.scala 214:29]
  assign when_DebugPlugin_l238 = (DebugPlugin_haltIt && (! DebugPlugin_isPipBusy)); // @[BaseType.scala 305:24]
  assign DebugPlugin_allowEBreak = (DebugPlugin_debugUsed && (! DebugPlugin_disableEbreak)); // @[BaseType.scala 305:24]
  always @(*) begin
    debug_bus_cmd_ready = 1'b1; // @[DebugPlugin.scala 255:24]
    if(debug_bus_cmd_valid) begin
      case(switch_DebugPlugin_l280)
        6'h01 : begin
          if(debug_bus_cmd_payload_wr) begin
            debug_bus_cmd_ready = IBusSimplePlugin_injectionPort_ready; // @[DebugPlugin.scala 294:32]
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
    IBusSimplePlugin_injectionPort_valid = 1'b0; // @[DebugPlugin.scala 276:27]
    if(debug_bus_cmd_valid) begin
      case(switch_DebugPlugin_l280)
        6'h01 : begin
          if(debug_bus_cmd_payload_wr) begin
            IBusSimplePlugin_injectionPort_valid = 1'b1; // @[DebugPlugin.scala 293:35]
          end
        end
        default : begin
        end
      endcase
    end
  end

  assign IBusSimplePlugin_injectionPort_payload = debug_bus_cmd_payload_data; // @[DebugPlugin.scala 277:29]
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
  assign when_DebugPlugin_l324 = (DebugPlugin_stepIt && IBusSimplePlugin_incomingInstruction); // @[BaseType.scala 305:24]
  assign debug_resetOut = DebugPlugin_resetIt_regNext; // @[DebugPlugin.scala 341:19]
  assign when_DebugPlugin_l344 = (DebugPlugin_haltIt || DebugPlugin_stepIt); // @[BaseType.scala 305:24]
  assign when_Pipeline_l124 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_1 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_2 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_3 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_4 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_5 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_6 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_7 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_8 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_9 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_10 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_SRC1_CTRL = _zz_decode_SRC1_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_11 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_12 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_13 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_14 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_SRC2_CTRL = _zz_decode_SRC2_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_15 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_16 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_17 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_18 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_19 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_20 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_21 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_22 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_23 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_to_execute_ENV_CTRL_1 = decode_ENV_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_execute_to_memory_ENV_CTRL_1 = execute_ENV_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_memory_to_writeBack_ENV_CTRL_1 = memory_ENV_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_ENV_CTRL = _zz_decode_ENV_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_24 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_ENV_CTRL = decode_to_execute_ENV_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_25 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_memory_ENV_CTRL = execute_to_memory_ENV_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_26 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_writeBack_ENV_CTRL = memory_to_writeBack_ENV_CTRL; // @[Pipeline.scala 124:26]
  assign _zz_decode_to_execute_ALU_CTRL_1 = decode_ALU_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_ALU_CTRL = _zz_decode_ALU_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_27 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_ALU_CTRL = decode_to_execute_ALU_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_28 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_to_execute_ALU_BITWISE_CTRL_1 = decode_ALU_BITWISE_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_ALU_BITWISE_CTRL = _zz_decode_ALU_BITWISE_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_29 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_ALU_BITWISE_CTRL = decode_to_execute_ALU_BITWISE_CTRL; // @[Pipeline.scala 124:26]
  assign _zz_decode_to_execute_SHIFT_CTRL_1 = decode_SHIFT_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_SHIFT_CTRL = _zz_decode_SHIFT_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_30 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_SHIFT_CTRL = decode_to_execute_SHIFT_CTRL; // @[Pipeline.scala 124:26]
  assign _zz_decode_to_execute_BRANCH_CTRL_1 = decode_BRANCH_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_BRANCH_CTRL = _zz_decode_BRANCH_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_31 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_BRANCH_CTRL = decode_to_execute_BRANCH_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_32 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_33 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_34 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_35 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_36 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_37 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_38 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_39 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_40 = ((! memory_arbitration_isStuck) && (! execute_arbitration_isStuckByOthers)); // @[BaseType.scala 305:24]
  assign when_Pipeline_l124_41 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_42 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_43 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_44 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
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
    IBusSimplePlugin_injectionPort_ready = 1'b0; // @[Fetcher.scala 361:31]
    case(switch_Fetcher_l365)
      3'b100 : begin
        IBusSimplePlugin_injectionPort_ready = 1'b1; // @[Fetcher.scala 386:35]
      end
      default : begin
      end
    endcase
  end

  assign when_Fetcher_l381 = (! decode_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Fetcher_l401 = (switch_Fetcher_l365 != 3'b000); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1589 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_1 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_2 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_3 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign switch_CsrPlugin_l980 = CsrPlugin_csrMapping_writeDataSignal[12 : 11]; // @[BaseType.scala 299:24]
  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_768) begin
      _zz_CsrPlugin_csrMapping_readDataInit[7 : 7] = CsrPlugin_mstatus_MPIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit[3 : 3] = CsrPlugin_mstatus_MIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit[12 : 11] = CsrPlugin_mstatus_MPP; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_1 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_836) begin
      _zz_CsrPlugin_csrMapping_readDataInit_1[11 : 11] = CsrPlugin_mip_MEIP; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_1[7 : 7] = CsrPlugin_mip_MTIP; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_1[3 : 3] = CsrPlugin_mip_MSIP; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_2 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_772) begin
      _zz_CsrPlugin_csrMapping_readDataInit_2[11 : 11] = CsrPlugin_mie_MEIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_2[7 : 7] = CsrPlugin_mie_MTIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_2[3 : 3] = CsrPlugin_mie_MSIE; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_3 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_834) begin
      _zz_CsrPlugin_csrMapping_readDataInit_3[31 : 31] = CsrPlugin_mcause_interrupt; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_3[3 : 0] = CsrPlugin_mcause_exceptionCode; // @[CsrPlugin.scala 1598:138]
    end
  end

  assign CsrPlugin_csrMapping_readDataInit = ((_zz_CsrPlugin_csrMapping_readDataInit | _zz_CsrPlugin_csrMapping_readDataInit_1) | (_zz_CsrPlugin_csrMapping_readDataInit_2 | _zz_CsrPlugin_csrMapping_readDataInit_3)); // @[CsrPlugin.scala 1604:39]
  always @(*) begin
    when_CsrPlugin_l1625 = 1'b0; // @[CsrPlugin.scala 1622:27]
    if(when_CsrPlugin_l1623) begin
      when_CsrPlugin_l1625 = 1'b1; // @[CsrPlugin.scala 1623:21]
    end
  end

  assign when_CsrPlugin_l1623 = (CsrPlugin_privilege < execute_CsrPlugin_csrAddress[9 : 8]); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1631 = ((! execute_arbitration_isValid) || (! execute_IS_CSR)); // @[BaseType.scala 305:24]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      IBusSimplePlugin_fetchPc_pcReg <= 32'h80000000; // @[Data.scala 400:33]
      IBusSimplePlugin_fetchPc_correctionReg <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_fetchPc_booted <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_fetchPc_inc <= 1'b0; // @[Data.scala 400:33]
      _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid_1 <= 1'b0; // @[Data.scala 400:33]
      _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid <= 1'b0; // @[Data.scala 400:33]
      _zz_IBusSimplePlugin_injector_decodeInput_valid <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_injector_nextPcCalc_valids_0 <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_injector_nextPcCalc_valids_1 <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_injector_nextPcCalc_valids_2 <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_injector_nextPcCalc_valids_3 <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_injector_nextPcCalc_valids_4 <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_injector_nextPcCalc_valids_5 <= 1'b0; // @[Data.scala 400:33]
      IBusSimplePlugin_pending_value <= 3'b000; // @[Data.scala 400:33]
      IBusSimplePlugin_rspJoin_rspBuffer_discardCounter <= 3'b000; // @[Data.scala 400:33]
      CsrPlugin_mstatus_MIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mstatus_MPIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mstatus_MPP <= 2'b11; // @[Data.scala 400:33]
      CsrPlugin_mie_MEIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mie_MTIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mie_MSIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mcycle <= 64'h0; // @[Data.scala 400:33]
      CsrPlugin_minstret <= 64'h0; // @[Data.scala 400:33]
      CsrPlugin_interrupt_valid <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_pipelineLiberator_pcValids_0 <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_pipelineLiberator_pcValids_1 <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_pipelineLiberator_pcValids_2 <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_hadException <= 1'b0; // @[Data.scala 400:33]
      execute_CsrPlugin_wfiWake <= 1'b0; // @[Data.scala 400:33]
      _zz_2 <= 1'b1; // @[Data.scala 400:33]
      execute_LightShifterPlugin_isActive <= 1'b0; // @[Data.scala 400:33]
      HazardSimplePlugin_writeBackBuffer_valid <= 1'b0; // @[Data.scala 400:33]
      execute_arbitration_isValid <= 1'b0; // @[Data.scala 400:33]
      memory_arbitration_isValid <= 1'b0; // @[Data.scala 400:33]
      writeBack_arbitration_isValid <= 1'b0; // @[Data.scala 400:33]
      switch_Fetcher_l365 <= 3'b000; // @[Data.scala 400:33]
    end else begin
      if(IBusSimplePlugin_fetchPc_correction) begin
        IBusSimplePlugin_fetchPc_correctionReg <= 1'b1; // @[Fetcher.scala 130:42]
      end
      if(IBusSimplePlugin_fetchPc_output_fire) begin
        IBusSimplePlugin_fetchPc_correctionReg <= 1'b0; // @[Fetcher.scala 130:62]
      end
      IBusSimplePlugin_fetchPc_booted <= 1'b1; // @[Reg.scala 39:30]
      if(when_Fetcher_l134) begin
        IBusSimplePlugin_fetchPc_inc <= 1'b0; // @[Fetcher.scala 134:32]
      end
      if(IBusSimplePlugin_fetchPc_output_fire_1) begin
        IBusSimplePlugin_fetchPc_inc <= 1'b1; // @[Fetcher.scala 134:72]
      end
      if(when_Fetcher_l134_1) begin
        IBusSimplePlugin_fetchPc_inc <= 1'b0; // @[Fetcher.scala 134:93]
      end
      if(when_Fetcher_l161) begin
        IBusSimplePlugin_fetchPc_pcReg <= IBusSimplePlugin_fetchPc_pc; // @[Fetcher.scala 162:15]
      end
      if(IBusSimplePlugin_iBusRsp_flush) begin
        _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid_1 <= 1'b0; // @[Misc.scala 146:41]
      end
      if(_zz_IBusSimplePlugin_iBusRsp_stages_0_output_ready) begin
        _zz_IBusSimplePlugin_iBusRsp_stages_1_input_valid_1 <= (IBusSimplePlugin_iBusRsp_stages_0_output_valid && (! 1'b0)); // @[Misc.scala 154:18]
      end
      if(IBusSimplePlugin_iBusRsp_flush) begin
        _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid <= 1'b0; // @[Misc.scala 146:41]
      end
      if(IBusSimplePlugin_iBusRsp_stages_1_output_ready) begin
        _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_valid <= (IBusSimplePlugin_iBusRsp_stages_1_output_valid && (! IBusSimplePlugin_iBusRsp_flush)); // @[Misc.scala 154:18]
      end
      if(decode_arbitration_removeIt) begin
        _zz_IBusSimplePlugin_injector_decodeInput_valid <= 1'b0; // @[Misc.scala 146:41]
      end
      if(IBusSimplePlugin_iBusRsp_output_ready) begin
        _zz_IBusSimplePlugin_injector_decodeInput_valid <= (IBusSimplePlugin_iBusRsp_output_valid && (! IBusSimplePlugin_externalFlush)); // @[Misc.scala 154:18]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_0 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_0 <= 1'b1; // @[Fetcher.scala 333:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_1 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_1) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_1 <= IBusSimplePlugin_injector_nextPcCalc_valids_0; // @[Fetcher.scala 333:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_1 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_2 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_2) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_2 <= IBusSimplePlugin_injector_nextPcCalc_valids_1; // @[Fetcher.scala 333:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_2 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_3 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_3) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_3 <= IBusSimplePlugin_injector_nextPcCalc_valids_2; // @[Fetcher.scala 333:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_3 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_4 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_4) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_4 <= IBusSimplePlugin_injector_nextPcCalc_valids_3; // @[Fetcher.scala 333:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_4 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_5 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_5) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_5 <= IBusSimplePlugin_injector_nextPcCalc_valids_4; // @[Fetcher.scala 333:17]
      end
      if(IBusSimplePlugin_fetchPc_flushed) begin
        IBusSimplePlugin_injector_nextPcCalc_valids_5 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      IBusSimplePlugin_pending_value <= IBusSimplePlugin_pending_next; // @[IBusSimplePlugin.scala 295:15]
      IBusSimplePlugin_rspJoin_rspBuffer_discardCounter <= (IBusSimplePlugin_rspJoin_rspBuffer_discardCounter - _zz_IBusSimplePlugin_rspJoin_rspBuffer_discardCounter); // @[IBusSimplePlugin.scala 357:26]
      if(IBusSimplePlugin_iBusRsp_flush) begin
        IBusSimplePlugin_rspJoin_rspBuffer_discardCounter <= IBusSimplePlugin_pending_next; // @[IBusSimplePlugin.scala 359:28]
      end
      CsrPlugin_mcycle <= (CsrPlugin_mcycle + 64'h0000000000000001); // @[CsrPlugin.scala 1096:14]
      if(writeBack_arbitration_isFiring) begin
        CsrPlugin_minstret <= (CsrPlugin_minstret + 64'h0000000000000001); // @[CsrPlugin.scala 1098:18]
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
          end
          default : begin
          end
        endcase
      end
      execute_CsrPlugin_wfiWake <= (({_zz_when_CsrPlugin_l1222_2,{_zz_when_CsrPlugin_l1222_1,_zz_when_CsrPlugin_l1222}} != 3'b000) || CsrPlugin_thirdPartyWake); // @[Reg.scala 39:30]
      _zz_2 <= 1'b0; // @[Reg.scala 39:30]
      if(when_ShiftPlugins_l169) begin
        if(when_ShiftPlugins_l175) begin
          execute_LightShifterPlugin_isActive <= 1'b1; // @[ShiftPlugins.scala 176:20]
          if(execute_LightShifterPlugin_done) begin
            execute_LightShifterPlugin_isActive <= 1'b0; // @[ShiftPlugins.scala 180:22]
          end
        end
      end
      if(execute_arbitration_removeIt) begin
        execute_LightShifterPlugin_isActive <= 1'b0; // @[ShiftPlugins.scala 189:18]
      end
      HazardSimplePlugin_writeBackBuffer_valid <= HazardSimplePlugin_writeBackWrites_valid; // @[Reg.scala 39:30]
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
          if(IBusSimplePlugin_injectionPort_valid) begin
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
      if(execute_CsrPlugin_csr_768) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_mstatus_MPIE <= CsrPlugin_csrMapping_writeDataSignal[7]; // @[Bool.scala 189:10]
          CsrPlugin_mstatus_MIE <= CsrPlugin_csrMapping_writeDataSignal[3]; // @[Bool.scala 189:10]
          case(switch_CsrPlugin_l980)
            2'b11 : begin
              CsrPlugin_mstatus_MPP <= 2'b11; // @[CsrPlugin.scala 981:30]
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
    end
  end

  always @(posedge io_mainClk) begin
    if(IBusSimplePlugin_iBusRsp_stages_1_output_ready) begin
      _zz_IBusSimplePlugin_iBusRsp_stages_1_output_m2sPipe_payload <= IBusSimplePlugin_iBusRsp_stages_1_output_payload; // @[Misc.scala 155:15]
    end
    if(IBusSimplePlugin_iBusRsp_output_ready) begin
      _zz_IBusSimplePlugin_injector_decodeInput_payload_pc <= IBusSimplePlugin_iBusRsp_output_payload_pc; // @[Misc.scala 155:15]
      _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_error <= IBusSimplePlugin_iBusRsp_output_payload_rsp_error; // @[Misc.scala 155:15]
      _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_inst <= IBusSimplePlugin_iBusRsp_output_payload_rsp_inst; // @[Misc.scala 155:15]
      _zz_IBusSimplePlugin_injector_decodeInput_payload_isRvc <= IBusSimplePlugin_iBusRsp_output_payload_isRvc; // @[Misc.scala 155:15]
    end
    if(IBusSimplePlugin_injector_decodeInput_ready) begin
      IBusSimplePlugin_injector_formal_rawInDecode <= IBusSimplePlugin_iBusRsp_output_payload_rsp_inst; // @[Utils.scala 1084:26]
    end
    CsrPlugin_mip_MEIP <= externalInterrupt; // @[Reg.scala 39:30]
    CsrPlugin_mip_MTIP <= timerInterrupt; // @[Reg.scala 39:30]
    CsrPlugin_mip_MSIP <= softwareInterrupt; // @[Reg.scala 39:30]
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
            CsrPlugin_mepc <= decode_PC; // @[CsrPlugin.scala 1339:20]
          end
          default : begin
          end
        endcase
      end
    end
    if(when_ShiftPlugins_l169) begin
      if(when_ShiftPlugins_l175) begin
        execute_LightShifterPlugin_amplitudeReg <= (execute_LightShifterPlugin_amplitude - 5'h01); // @[ShiftPlugins.scala 177:24]
      end
    end
    HazardSimplePlugin_writeBackBuffer_payload_address <= HazardSimplePlugin_writeBackWrites_payload_address; // @[Reg.scala 39:30]
    HazardSimplePlugin_writeBackBuffer_payload_data <= HazardSimplePlugin_writeBackWrites_payload_data; // @[Reg.scala 39:30]
    if(when_Pipeline_l124) begin
      decode_to_execute_PC <= _zz_decode_to_execute_PC; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_1) begin
      execute_to_memory_PC <= execute_PC; // @[Pipeline.scala 124:40]
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
      decode_to_execute_FORMAL_PC_NEXT <= decode_FORMAL_PC_NEXT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_7) begin
      execute_to_memory_FORMAL_PC_NEXT <= execute_FORMAL_PC_NEXT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_8) begin
      memory_to_writeBack_FORMAL_PC_NEXT <= _zz_memory_to_writeBack_FORMAL_PC_NEXT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_9) begin
      decode_to_execute_CSR_WRITE_OPCODE <= decode_CSR_WRITE_OPCODE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_10) begin
      decode_to_execute_CSR_READ_OPCODE <= decode_CSR_READ_OPCODE; // @[Pipeline.scala 124:40]
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
      decode_to_execute_REGFILE_WRITE_VALID <= decode_REGFILE_WRITE_VALID; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_16) begin
      execute_to_memory_REGFILE_WRITE_VALID <= execute_REGFILE_WRITE_VALID; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_17) begin
      memory_to_writeBack_REGFILE_WRITE_VALID <= memory_REGFILE_WRITE_VALID; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_18) begin
      decode_to_execute_BYPASSABLE_EXECUTE_STAGE <= decode_BYPASSABLE_EXECUTE_STAGE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_19) begin
      decode_to_execute_BYPASSABLE_MEMORY_STAGE <= decode_BYPASSABLE_MEMORY_STAGE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_20) begin
      execute_to_memory_BYPASSABLE_MEMORY_STAGE <= execute_BYPASSABLE_MEMORY_STAGE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_21) begin
      decode_to_execute_MEMORY_STORE <= decode_MEMORY_STORE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_22) begin
      execute_to_memory_MEMORY_STORE <= execute_MEMORY_STORE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_23) begin
      decode_to_execute_IS_CSR <= decode_IS_CSR; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_24) begin
      decode_to_execute_ENV_CTRL <= _zz_decode_to_execute_ENV_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_25) begin
      execute_to_memory_ENV_CTRL <= _zz_execute_to_memory_ENV_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_26) begin
      memory_to_writeBack_ENV_CTRL <= _zz_memory_to_writeBack_ENV_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_27) begin
      decode_to_execute_ALU_CTRL <= _zz_decode_to_execute_ALU_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_28) begin
      decode_to_execute_SRC_LESS_UNSIGNED <= decode_SRC_LESS_UNSIGNED; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_29) begin
      decode_to_execute_ALU_BITWISE_CTRL <= _zz_decode_to_execute_ALU_BITWISE_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_30) begin
      decode_to_execute_SHIFT_CTRL <= _zz_decode_to_execute_SHIFT_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_31) begin
      decode_to_execute_BRANCH_CTRL <= _zz_decode_to_execute_BRANCH_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_32) begin
      decode_to_execute_RS1 <= _zz_decode_to_execute_RS1; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_33) begin
      decode_to_execute_RS2 <= _zz_decode_to_execute_RS2; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_34) begin
      decode_to_execute_SRC2_FORCE_ZERO <= decode_SRC2_FORCE_ZERO; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_35) begin
      decode_to_execute_SRC1 <= decode_SRC1; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_36) begin
      decode_to_execute_SRC2 <= decode_SRC2; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_37) begin
      decode_to_execute_DO_EBREAK <= decode_DO_EBREAK; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_38) begin
      execute_to_memory_MEMORY_ADDRESS_LOW <= execute_MEMORY_ADDRESS_LOW; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_39) begin
      memory_to_writeBack_MEMORY_ADDRESS_LOW <= memory_MEMORY_ADDRESS_LOW; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_40) begin
      execute_to_memory_REGFILE_WRITE_DATA <= _zz_decode_RS2_1; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_41) begin
      memory_to_writeBack_REGFILE_WRITE_DATA <= _zz_decode_RS2; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_42) begin
      execute_to_memory_BRANCH_DO <= execute_BRANCH_DO; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_43) begin
      execute_to_memory_BRANCH_CALC <= execute_BRANCH_CALC; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_44) begin
      memory_to_writeBack_MEMORY_READ_DATA <= memory_MEMORY_READ_DATA; // @[Pipeline.scala 124:40]
    end
    if(when_Fetcher_l401) begin
      _zz_IBusSimplePlugin_injector_decodeInput_payload_rsp_inst <= IBusSimplePlugin_injectionPort_payload; // @[Fetcher.scala 402:35]
    end
    if(when_CsrPlugin_l1589) begin
      execute_CsrPlugin_csr_768 <= (decode_INSTRUCTION[31 : 20] == 12'h300); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_1) begin
      execute_CsrPlugin_csr_836 <= (decode_INSTRUCTION[31 : 20] == 12'h344); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_2) begin
      execute_CsrPlugin_csr_772 <= (decode_INSTRUCTION[31 : 20] == 12'h304); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_3) begin
      execute_CsrPlugin_csr_834 <= (decode_INSTRUCTION[31 : 20] == 12'h342); // @[CsrPlugin.scala 1589:101]
    end
    if(execute_CsrPlugin_csr_836) begin
      if(execute_CsrPlugin_writeEnable) begin
        CsrPlugin_mip_MSIP <= CsrPlugin_csrMapping_writeDataSignal[3]; // @[Bool.scala 189:10]
      end
    end
  end

  always @(posedge io_mainClk) begin
    DebugPlugin_firstCycle <= 1'b0; // @[Reg.scala 39:30]
    if(debug_bus_cmd_ready) begin
      DebugPlugin_firstCycle <= 1'b1; // @[DebugPlugin.scala 231:39]
    end
    DebugPlugin_secondCycle <= DebugPlugin_firstCycle; // @[Reg.scala 39:30]
    DebugPlugin_isPipBusy <= (({writeBack_arbitration_isValid,{memory_arbitration_isValid,{execute_arbitration_isValid,decode_arbitration_isValid}}} != 4'b0000) || IBusSimplePlugin_incomingInstruction); // @[Reg.scala 39:30]
    if(writeBack_arbitration_isValid) begin
      DebugPlugin_busReadDataReg <= _zz_decode_RS2_2; // @[DebugPlugin.scala 253:24]
    end
    _zz_when_DebugPlugin_l257 <= debug_bus_cmd_payload_address[2]; // @[Reg.scala 39:30]
    if(when_DebugPlugin_l308) begin
      DebugPlugin_busReadDataReg <= execute_PC; // @[DebugPlugin.scala 310:24]
    end
    DebugPlugin_resetIt_regNext <= DebugPlugin_resetIt; // @[Reg.scala 39:30]
  end

  always @(posedge io_mainClk or posedge resetCtrl_mainClkReset) begin
    if(resetCtrl_mainClkReset) begin
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

module MuraxMasterArbiter (
  input               io_iBus_cmd_valid,
  output reg          io_iBus_cmd_ready,
  input      [31:0]   io_iBus_cmd_payload_pc,
  output              io_iBus_rsp_valid,
  output              io_iBus_rsp_payload_error,
  output     [31:0]   io_iBus_rsp_payload_inst,
  input               io_dBus_cmd_valid,
  output reg          io_dBus_cmd_ready,
  input               io_dBus_cmd_payload_wr,
  input      [31:0]   io_dBus_cmd_payload_address,
  input      [31:0]   io_dBus_cmd_payload_data,
  input      [1:0]    io_dBus_cmd_payload_size,
  output              io_dBus_rsp_ready,
  output              io_dBus_rsp_error,
  output     [31:0]   io_dBus_rsp_data,
  output reg          io_masterBus_cmd_valid,
  input               io_masterBus_cmd_ready,
  output              io_masterBus_cmd_payload_write,
  output     [31:0]   io_masterBus_cmd_payload_address,
  output     [31:0]   io_masterBus_cmd_payload_data,
  output     [3:0]    io_masterBus_cmd_payload_mask,
  input               io_masterBus_rsp_valid,
  input      [31:0]   io_masterBus_rsp_payload_data,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg        [3:0]    _zz_io_masterBus_cmd_payload_mask;
  reg                 rspPending;
  reg                 rspTarget;
  wire                io_masterBus_cmd_fire;
  wire                when_MuraxUtiles_l31;
  wire                when_MuraxUtiles_l36;

  always @(*) begin
    io_masterBus_cmd_valid = (io_iBus_cmd_valid || io_dBus_cmd_valid); // @[MuraxUtiles.scala 20:28]
    if(when_MuraxUtiles_l36) begin
      io_masterBus_cmd_valid = 1'b0; // @[MuraxUtiles.scala 39:28]
    end
  end

  assign io_masterBus_cmd_payload_write = (io_dBus_cmd_valid && io_dBus_cmd_payload_wr); // @[MuraxUtiles.scala 21:31]
  assign io_masterBus_cmd_payload_address = (io_dBus_cmd_valid ? io_dBus_cmd_payload_address : io_iBus_cmd_payload_pc); // @[MuraxUtiles.scala 22:28]
  assign io_masterBus_cmd_payload_data = io_dBus_cmd_payload_data; // @[MuraxUtiles.scala 23:28]
  always @(*) begin
    case(io_dBus_cmd_payload_size)
      2'b00 : begin
        _zz_io_masterBus_cmd_payload_mask = 4'b0001; // @[Misc.scala 239:22]
      end
      2'b01 : begin
        _zz_io_masterBus_cmd_payload_mask = 4'b0011; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_io_masterBus_cmd_payload_mask = 4'b1111; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign io_masterBus_cmd_payload_mask = (_zz_io_masterBus_cmd_payload_mask <<< io_dBus_cmd_payload_address[1 : 0]); // @[MuraxUtiles.scala 24:28]
  always @(*) begin
    io_iBus_cmd_ready = (io_masterBus_cmd_ready && (! io_dBus_cmd_valid)); // @[MuraxUtiles.scala 25:21]
    if(when_MuraxUtiles_l36) begin
      io_iBus_cmd_ready = 1'b0; // @[MuraxUtiles.scala 37:23]
    end
  end

  always @(*) begin
    io_dBus_cmd_ready = io_masterBus_cmd_ready; // @[MuraxUtiles.scala 26:21]
    if(when_MuraxUtiles_l36) begin
      io_dBus_cmd_ready = 1'b0; // @[MuraxUtiles.scala 38:23]
    end
  end

  assign io_masterBus_cmd_fire = (io_masterBus_cmd_valid && io_masterBus_cmd_ready); // @[BaseType.scala 305:24]
  assign when_MuraxUtiles_l31 = (io_masterBus_cmd_fire && (! io_masterBus_cmd_payload_write)); // @[BaseType.scala 305:24]
  assign when_MuraxUtiles_l36 = (rspPending && (! io_masterBus_rsp_valid)); // @[BaseType.scala 305:24]
  assign io_iBus_rsp_valid = (io_masterBus_rsp_valid && (! rspTarget)); // @[MuraxUtiles.scala 42:21]
  assign io_iBus_rsp_payload_inst = io_masterBus_rsp_payload_data; // @[MuraxUtiles.scala 43:21]
  assign io_iBus_rsp_payload_error = 1'b0; // @[MuraxUtiles.scala 44:21]
  assign io_dBus_rsp_ready = (io_masterBus_rsp_valid && rspTarget); // @[MuraxUtiles.scala 46:21]
  assign io_dBus_rsp_data = io_masterBus_rsp_payload_data; // @[MuraxUtiles.scala 47:21]
  assign io_dBus_rsp_error = 1'b0; // @[MuraxUtiles.scala 48:21]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      rspPending <= 1'b0; // @[Data.scala 400:33]
      rspTarget <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(io_masterBus_rsp_valid) begin
        rspPending <= 1'b0; // @[MuraxUtiles.scala 29:35]
      end
      if(when_MuraxUtiles_l31) begin
        rspTarget <= io_dBus_cmd_valid; // @[MuraxUtiles.scala 32:16]
        rspPending <= 1'b1; // @[MuraxUtiles.scala 33:16]
      end
    end
  end


endmodule

module BufferCC (
  input               io_dataIn,
  output              io_dataOut,
  input               io_mainClk
);

  (* async_reg = "true" *) reg                 buffers_0;
  (* async_reg = "true" *) reg                 buffers_1;

  assign io_dataOut = buffers_1; // @[CrossClock.scala 38:14]
  always @(posedge io_mainClk) begin
    buffers_0 <= io_dataIn; // @[CrossClock.scala 31:14]
    buffers_1 <= buffers_0; // @[CrossClock.scala 34:16]
  end


endmodule

module InterruptCtrl (
  input      [1:0]    io_inputs,
  input      [1:0]    io_clears,
  input      [1:0]    io_masks,
  output     [1:0]    io_pendings,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg        [1:0]    pendings;

  assign io_pendings = (pendings & io_masks); // @[InterruptCtrl.scala 18:15]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      pendings <= 2'b00; // @[Data.scala 400:33]
    end else begin
      pendings <= ((pendings & (~ io_clears)) | io_inputs); // @[InterruptCtrl.scala 16:12]
    end
  end


endmodule

//Timer_1 replaced by Timer_1

module Timer_1 (
  input               io_tick,
  input               io_clear,
  input      [15:0]   io_limit,
  output              io_full,
  output     [15:0]   io_value,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  wire       [15:0]   _zz_counter;
  wire       [0:0]    _zz_counter_1;
  reg        [15:0]   counter;
  wire                limitHit;
  reg                 inhibitFull;

  assign _zz_counter_1 = (! limitHit);
  assign _zz_counter = {15'd0, _zz_counter_1};
  assign limitHit = (counter == io_limit); // @[BaseType.scala 305:24]
  assign io_full = ((limitHit && io_tick) && (! inhibitFull)); // @[Timer.scala 27:12]
  assign io_value = counter; // @[Timer.scala 28:12]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      inhibitFull <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(io_tick) begin
        inhibitFull <= limitHit; // @[Timer.scala 20:17]
      end
      if(io_clear) begin
        inhibitFull <= 1'b0; // @[Timer.scala 25:17]
      end
    end
  end

  always @(posedge io_mainClk) begin
    if(io_tick) begin
      counter <= (counter + _zz_counter); // @[Timer.scala 21:13]
    end
    if(io_clear) begin
      counter <= 16'h0; // @[Timer.scala 24:13]
    end
  end


endmodule

module Prescaler (
  input               io_clear,
  input      [15:0]   io_limit,
  output              io_overflow,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg        [15:0]   counter;
  wire                when_Prescaler_l17;

  assign when_Prescaler_l17 = (io_clear || io_overflow); // @[BaseType.scala 305:24]
  assign io_overflow = (counter == io_limit); // @[Prescaler.scala 21:15]
  always @(posedge io_mainClk) begin
    counter <= (counter + 16'h0001); // @[Prescaler.scala 15:11]
    if(when_Prescaler_l17) begin
      counter <= 16'h0; // @[Prescaler.scala 18:13]
    end
  end


endmodule

//StreamFifo_1 replaced by StreamFifo_1

module StreamFifo_1 (
  input               io_push_valid,
  output              io_push_ready,
  input      [7:0]    io_push_payload,
  output              io_pop_valid,
  input               io_pop_ready,
  output     [7:0]    io_pop_payload,
  input               io_flush,
  output     [4:0]    io_occupancy,
  output     [4:0]    io_availability,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg        [7:0]    _zz_logic_ram_port0;
  wire       [3:0]    _zz_logic_pushPtr_valueNext;
  wire       [0:0]    _zz_logic_pushPtr_valueNext_1;
  wire       [3:0]    _zz_logic_popPtr_valueNext;
  wire       [0:0]    _zz_logic_popPtr_valueNext_1;
  wire                _zz_logic_ram_port;
  wire                _zz_io_pop_payload;
  wire       [3:0]    _zz_io_availability;
  reg                 _zz_1;
  reg                 logic_pushPtr_willIncrement;
  reg                 logic_pushPtr_willClear;
  reg        [3:0]    logic_pushPtr_valueNext;
  reg        [3:0]    logic_pushPtr_value;
  wire                logic_pushPtr_willOverflowIfInc;
  wire                logic_pushPtr_willOverflow;
  reg                 logic_popPtr_willIncrement;
  reg                 logic_popPtr_willClear;
  reg        [3:0]    logic_popPtr_valueNext;
  reg        [3:0]    logic_popPtr_value;
  wire                logic_popPtr_willOverflowIfInc;
  wire                logic_popPtr_willOverflow;
  wire                logic_ptrMatch;
  reg                 logic_risingOccupancy;
  wire                logic_pushing;
  wire                logic_popping;
  wire                logic_empty;
  wire                logic_full;
  reg                 _zz_io_pop_valid;
  wire                when_Stream_l1101;
  wire       [3:0]    logic_ptrDif;
  reg [7:0] logic_ram [0:15];

  assign _zz_logic_pushPtr_valueNext_1 = logic_pushPtr_willIncrement;
  assign _zz_logic_pushPtr_valueNext = {3'd0, _zz_logic_pushPtr_valueNext_1};
  assign _zz_logic_popPtr_valueNext_1 = logic_popPtr_willIncrement;
  assign _zz_logic_popPtr_valueNext = {3'd0, _zz_logic_popPtr_valueNext_1};
  assign _zz_io_availability = (logic_popPtr_value - logic_pushPtr_value);
  assign _zz_io_pop_payload = 1'b1;
  always @(posedge io_mainClk) begin
    if(_zz_io_pop_payload) begin
      _zz_logic_ram_port0 <= logic_ram[logic_popPtr_valueNext];
    end
  end

  always @(posedge io_mainClk) begin
    if(_zz_1) begin
      logic_ram[logic_pushPtr_value] <= io_push_payload;
    end
  end

  always @(*) begin
    _zz_1 = 1'b0; // @[when.scala 47:16]
    if(logic_pushing) begin
      _zz_1 = 1'b1; // @[when.scala 52:10]
    end
  end

  always @(*) begin
    logic_pushPtr_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(logic_pushing) begin
      logic_pushPtr_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  always @(*) begin
    logic_pushPtr_willClear = 1'b0; // @[Utils.scala 537:19]
    if(io_flush) begin
      logic_pushPtr_willClear = 1'b1; // @[Utils.scala 539:33]
    end
  end

  assign logic_pushPtr_willOverflowIfInc = (logic_pushPtr_value == 4'b1111); // @[BaseType.scala 305:24]
  assign logic_pushPtr_willOverflow = (logic_pushPtr_willOverflowIfInc && logic_pushPtr_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    logic_pushPtr_valueNext = (logic_pushPtr_value + _zz_logic_pushPtr_valueNext); // @[Utils.scala 548:15]
    if(logic_pushPtr_willClear) begin
      logic_pushPtr_valueNext = 4'b0000; // @[Utils.scala 558:15]
    end
  end

  always @(*) begin
    logic_popPtr_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(logic_popping) begin
      logic_popPtr_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  always @(*) begin
    logic_popPtr_willClear = 1'b0; // @[Utils.scala 537:19]
    if(io_flush) begin
      logic_popPtr_willClear = 1'b1; // @[Utils.scala 539:33]
    end
  end

  assign logic_popPtr_willOverflowIfInc = (logic_popPtr_value == 4'b1111); // @[BaseType.scala 305:24]
  assign logic_popPtr_willOverflow = (logic_popPtr_willOverflowIfInc && logic_popPtr_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    logic_popPtr_valueNext = (logic_popPtr_value + _zz_logic_popPtr_valueNext); // @[Utils.scala 548:15]
    if(logic_popPtr_willClear) begin
      logic_popPtr_valueNext = 4'b0000; // @[Utils.scala 558:15]
    end
  end

  assign logic_ptrMatch = (logic_pushPtr_value == logic_popPtr_value); // @[BaseType.scala 305:24]
  assign logic_pushing = (io_push_valid && io_push_ready); // @[BaseType.scala 305:24]
  assign logic_popping = (io_pop_valid && io_pop_ready); // @[BaseType.scala 305:24]
  assign logic_empty = (logic_ptrMatch && (! logic_risingOccupancy)); // @[BaseType.scala 305:24]
  assign logic_full = (logic_ptrMatch && logic_risingOccupancy); // @[BaseType.scala 305:24]
  assign io_push_ready = (! logic_full); // @[Stream.scala 1097:19]
  assign io_pop_valid = ((! logic_empty) && (! (_zz_io_pop_valid && (! logic_full)))); // @[Stream.scala 1098:18]
  assign io_pop_payload = _zz_logic_ram_port0; // @[Stream.scala 1099:20]
  assign when_Stream_l1101 = (logic_pushing != logic_popping); // @[BaseType.scala 305:24]
  assign logic_ptrDif = (logic_pushPtr_value - logic_popPtr_value); // @[BaseType.scala 299:24]
  assign io_occupancy = {(logic_risingOccupancy && logic_ptrMatch),logic_ptrDif}; // @[Stream.scala 1114:20]
  assign io_availability = {((! logic_risingOccupancy) && logic_ptrMatch),_zz_io_availability}; // @[Stream.scala 1115:23]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      logic_pushPtr_value <= 4'b0000; // @[Data.scala 400:33]
      logic_popPtr_value <= 4'b0000; // @[Data.scala 400:33]
      logic_risingOccupancy <= 1'b0; // @[Data.scala 400:33]
      _zz_io_pop_valid <= 1'b0; // @[Data.scala 400:33]
    end else begin
      logic_pushPtr_value <= logic_pushPtr_valueNext; // @[Reg.scala 39:30]
      logic_popPtr_value <= logic_popPtr_valueNext; // @[Reg.scala 39:30]
      _zz_io_pop_valid <= (logic_popPtr_valueNext == logic_pushPtr_value); // @[Reg.scala 39:30]
      if(when_Stream_l1101) begin
        logic_risingOccupancy <= logic_pushing; // @[Stream.scala 1102:23]
      end
      if(io_flush) begin
        logic_risingOccupancy <= 1'b0; // @[Stream.scala 1129:23]
      end
    end
  end


endmodule

module UartCtrl (
  input      [2:0]    io_config_frame_dataLength,
  input      [0:0]    io_config_frame_stop,
  input      [1:0]    io_config_frame_parity,
  input      [19:0]   io_config_clockDivider,
  input               io_write_valid,
  output reg          io_write_ready,
  input      [7:0]    io_write_payload,
  output              io_read_valid,
  input               io_read_ready,
  output     [7:0]    io_read_payload,
  output              io_uart_txd,
  input               io_uart_rxd,
  output              io_readError,
  input               io_writeBreak,
  output              io_readBreak,
  input               io_mainClk,
  input               resetCtrl_systemReset
);
  localparam UartStopType_ONE = 1'd0;
  localparam UartStopType_TWO = 1'd1;
  localparam UartParityType_NONE = 2'd0;
  localparam UartParityType_EVEN = 2'd1;
  localparam UartParityType_ODD = 2'd2;

  wire                tx_io_write_ready;
  wire                tx_io_txd;
  wire                rx_io_read_valid;
  wire       [7:0]    rx_io_read_payload;
  wire                rx_io_rts;
  wire                rx_io_error;
  wire                rx_io_break;
  reg        [19:0]   clockDivider_counter;
  wire                clockDivider_tick;
  reg                 clockDivider_tickReg;
  reg                 io_write_thrown_valid;
  wire                io_write_thrown_ready;
  wire       [7:0]    io_write_thrown_payload;
  `ifndef SYNTHESIS
  reg [23:0] io_config_frame_stop_string;
  reg [31:0] io_config_frame_parity_string;
  `endif


  UartCtrlTx tx (
    .io_configFrame_dataLength (io_config_frame_dataLength[2:0]), //i
    .io_configFrame_stop       (io_config_frame_stop           ), //i
    .io_configFrame_parity     (io_config_frame_parity[1:0]    ), //i
    .io_samplingTick           (clockDivider_tickReg           ), //i
    .io_write_valid            (io_write_thrown_valid          ), //i
    .io_write_ready            (tx_io_write_ready              ), //o
    .io_write_payload          (io_write_thrown_payload[7:0]   ), //i
    .io_cts                    (1'b0                           ), //i
    .io_txd                    (tx_io_txd                      ), //o
    .io_break                  (io_writeBreak                  ), //i
    .io_mainClk                (io_mainClk                     ), //i
    .resetCtrl_systemReset     (resetCtrl_systemReset          )  //i
  );
  UartCtrlRx rx (
    .io_configFrame_dataLength (io_config_frame_dataLength[2:0]), //i
    .io_configFrame_stop       (io_config_frame_stop           ), //i
    .io_configFrame_parity     (io_config_frame_parity[1:0]    ), //i
    .io_samplingTick           (clockDivider_tickReg           ), //i
    .io_read_valid             (rx_io_read_valid               ), //o
    .io_read_ready             (io_read_ready                  ), //i
    .io_read_payload           (rx_io_read_payload[7:0]        ), //o
    .io_rxd                    (io_uart_rxd                    ), //i
    .io_rts                    (rx_io_rts                      ), //o
    .io_error                  (rx_io_error                    ), //o
    .io_break                  (rx_io_break                    ), //o
    .io_mainClk                (io_mainClk                     ), //i
    .resetCtrl_systemReset     (resetCtrl_systemReset          )  //i
  );
  `ifndef SYNTHESIS
  always @(*) begin
    case(io_config_frame_stop)
      UartStopType_ONE : io_config_frame_stop_string = "ONE";
      UartStopType_TWO : io_config_frame_stop_string = "TWO";
      default : io_config_frame_stop_string = "???";
    endcase
  end
  always @(*) begin
    case(io_config_frame_parity)
      UartParityType_NONE : io_config_frame_parity_string = "NONE";
      UartParityType_EVEN : io_config_frame_parity_string = "EVEN";
      UartParityType_ODD : io_config_frame_parity_string = "ODD ";
      default : io_config_frame_parity_string = "????";
    endcase
  end
  `endif

  assign clockDivider_tick = (clockDivider_counter == 20'h0); // @[BaseType.scala 305:24]
  always @(*) begin
    io_write_thrown_valid = io_write_valid; // @[Stream.scala 294:16]
    if(rx_io_break) begin
      io_write_thrown_valid = 1'b0; // @[Stream.scala 439:18]
    end
  end

  always @(*) begin
    io_write_ready = io_write_thrown_ready; // @[Stream.scala 295:16]
    if(rx_io_break) begin
      io_write_ready = 1'b1; // @[Stream.scala 440:18]
    end
  end

  assign io_write_thrown_payload = io_write_payload; // @[Stream.scala 296:18]
  assign io_write_thrown_ready = tx_io_write_ready; // @[Stream.scala 295:16]
  assign io_read_valid = rx_io_read_valid; // @[Stream.scala 294:16]
  assign io_read_payload = rx_io_read_payload; // @[Stream.scala 296:18]
  assign io_uart_txd = tx_io_txd; // @[UartCtrl.scala 76:15]
  assign io_readError = rx_io_error; // @[UartCtrl.scala 79:16]
  assign io_readBreak = rx_io_break; // @[UartCtrl.scala 82:16]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      clockDivider_counter <= 20'h0; // @[Data.scala 400:33]
      clockDivider_tickReg <= 1'b0; // @[Data.scala 400:33]
    end else begin
      clockDivider_tickReg <= clockDivider_tick; // @[Reg.scala 39:30]
      clockDivider_counter <= (clockDivider_counter - 20'h00001); // @[UartCtrl.scala 61:13]
      if(clockDivider_tick) begin
        clockDivider_counter <= io_config_clockDivider; // @[UartCtrl.scala 63:15]
      end
    end
  end


endmodule

module BufferCC_1 (
  input      [31:0]   io_dataIn,
  output     [31:0]   io_dataOut,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  (* async_reg = "true" *) reg        [31:0]   buffers_0;
  (* async_reg = "true" *) reg        [31:0]   buffers_1;

  assign io_dataOut = buffers_1; // @[CrossClock.scala 38:14]
  always @(posedge io_mainClk) begin
    buffers_0 <= io_dataIn; // @[CrossClock.scala 31:14]
    buffers_1 <= buffers_0; // @[CrossClock.scala 34:16]
  end


endmodule

module FlowCCByToggle (
  input               io_input_valid,
  input               io_input_payload_last,
  input      [0:0]    io_input_payload_fragment,
  output              io_output_valid,
  output              io_output_payload_last,
  output     [0:0]    io_output_payload_fragment,
  input               io_jtag_tck,
  input               io_mainClk,
  input               resetCtrl_mainClkReset
);

  wire                inputArea_target_buffercc_io_dataOut;
  reg                 inputArea_target;
  reg                 inputArea_data_last;
  reg        [0:0]    inputArea_data_fragment;
  wire                outputArea_target;
  reg                 outputArea_hit;
  wire                outputArea_flow_valid;
  wire                outputArea_flow_payload_last;
  wire       [0:0]    outputArea_flow_payload_fragment;
  reg                 outputArea_flow_m2sPipe_valid;
  reg                 outputArea_flow_m2sPipe_payload_last;
  reg        [0:0]    outputArea_flow_m2sPipe_payload_fragment;

  BufferCC_2 inputArea_target_buffercc (
    .io_dataIn              (inputArea_target                    ), //i
    .io_dataOut             (inputArea_target_buffercc_io_dataOut), //o
    .io_mainClk             (io_mainClk                          ), //i
    .resetCtrl_mainClkReset (resetCtrl_mainClkReset              )  //i
  );
  initial begin
  `ifndef SYNTHESIS
    inputArea_target = $urandom;
    outputArea_hit = $urandom;
  `endif
  end

  assign outputArea_target = inputArea_target_buffercc_io_dataOut; // @[CrossClock.scala 13:9]
  assign outputArea_flow_valid = (outputArea_target != outputArea_hit); // @[Flow.scala 225:16]
  assign outputArea_flow_payload_last = inputArea_data_last; // @[Flow.scala 226:18]
  assign outputArea_flow_payload_fragment = inputArea_data_fragment; // @[Flow.scala 226:18]
  assign io_output_valid = outputArea_flow_m2sPipe_valid; // @[Flow.scala 94:11]
  assign io_output_payload_last = outputArea_flow_m2sPipe_payload_last; // @[Flow.scala 95:13]
  assign io_output_payload_fragment = outputArea_flow_m2sPipe_payload_fragment; // @[Flow.scala 95:13]
  always @(posedge io_jtag_tck) begin
    if(io_input_valid) begin
      inputArea_target <= (! inputArea_target); // @[Flow.scala 215:14]
      inputArea_data_last <= io_input_payload_last; // @[Flow.scala 216:12]
      inputArea_data_fragment <= io_input_payload_fragment; // @[Flow.scala 216:12]
    end
  end

  always @(posedge io_mainClk) begin
    outputArea_hit <= outputArea_target; // @[Reg.scala 39:30]
    if(outputArea_flow_valid) begin
      outputArea_flow_m2sPipe_payload_last <= outputArea_flow_payload_last; // @[Flow.scala 139:21]
      outputArea_flow_m2sPipe_payload_fragment <= outputArea_flow_payload_fragment; // @[Flow.scala 139:21]
    end
  end

  always @(posedge io_mainClk or posedge resetCtrl_mainClkReset) begin
    if(resetCtrl_mainClkReset) begin
      outputArea_flow_m2sPipe_valid <= 1'b0; // @[Data.scala 400:33]
    end else begin
      outputArea_flow_m2sPipe_valid <= outputArea_flow_valid; // @[Flow.scala 137:17]
    end
  end


endmodule

module StreamFifoLowLatency (
  input               io_push_valid,
  output              io_push_ready,
  input               io_push_payload_error,
  input      [31:0]   io_push_payload_inst,
  output reg          io_pop_valid,
  input               io_pop_ready,
  output reg          io_pop_payload_error,
  output reg [31:0]   io_pop_payload_inst,
  input               io_flush,
  output     [0:0]    io_occupancy,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  reg                 when_Phase_l648;
  reg                 pushPtr_willIncrement;
  reg                 pushPtr_willClear;
  wire                pushPtr_willOverflowIfInc;
  wire                pushPtr_willOverflow;
  reg                 popPtr_willIncrement;
  reg                 popPtr_willClear;
  wire                popPtr_willOverflowIfInc;
  wire                popPtr_willOverflow;
  wire                ptrMatch;
  reg                 risingOccupancy;
  wire                empty;
  wire                full;
  wire                pushing;
  wire                popping;
  wire                readed_error;
  wire       [31:0]   readed_inst;
  wire       [32:0]   _zz_readed_error;
  wire                when_Stream_l1196;
  wire                when_Stream_l1209;
  wire       [32:0]   _zz_readed_error_1;
  reg        [32:0]   _zz_readed_error_2;

  always @(*) begin
    when_Phase_l648 = 1'b0; // @[when.scala 47:16]
    if(pushing) begin
      when_Phase_l648 = 1'b1; // @[when.scala 52:10]
    end
  end

  always @(*) begin
    pushPtr_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(pushing) begin
      pushPtr_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  always @(*) begin
    pushPtr_willClear = 1'b0; // @[Utils.scala 537:19]
    if(io_flush) begin
      pushPtr_willClear = 1'b1; // @[Utils.scala 539:33]
    end
  end

  assign pushPtr_willOverflowIfInc = 1'b1; // @[BaseType.scala 305:24]
  assign pushPtr_willOverflow = (pushPtr_willOverflowIfInc && pushPtr_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    popPtr_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(popping) begin
      popPtr_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  always @(*) begin
    popPtr_willClear = 1'b0; // @[Utils.scala 537:19]
    if(io_flush) begin
      popPtr_willClear = 1'b1; // @[Utils.scala 539:33]
    end
  end

  assign popPtr_willOverflowIfInc = 1'b1; // @[BaseType.scala 305:24]
  assign popPtr_willOverflow = (popPtr_willOverflowIfInc && popPtr_willIncrement); // @[BaseType.scala 305:24]
  assign ptrMatch = 1'b1; // @[BaseType.scala 305:24]
  assign empty = (ptrMatch && (! risingOccupancy)); // @[BaseType.scala 305:24]
  assign full = (ptrMatch && risingOccupancy); // @[BaseType.scala 305:24]
  assign pushing = (io_push_valid && io_push_ready); // @[BaseType.scala 305:24]
  assign popping = (io_pop_valid && io_pop_ready); // @[BaseType.scala 305:24]
  assign io_push_ready = (! full); // @[Stream.scala 1190:17]
  assign _zz_readed_error = _zz_readed_error_1; // @[Mem.scala 285:24]
  assign readed_error = _zz_readed_error[0]; // @[Bool.scala 189:10]
  assign readed_inst = _zz_readed_error[32 : 1]; // @[Bits.scala 133:56]
  assign when_Stream_l1196 = (! empty); // @[BaseType.scala 299:24]
  always @(*) begin
    if(when_Stream_l1196) begin
      io_pop_valid = 1'b1; // @[Stream.scala 1197:22]
    end else begin
      io_pop_valid = io_push_valid; // @[Stream.scala 1200:22]
    end
  end

  always @(*) begin
    if(when_Stream_l1196) begin
      io_pop_payload_error = readed_error; // @[Stream.scala 1198:24]
    end else begin
      io_pop_payload_error = io_push_payload_error; // @[Stream.scala 1201:24]
    end
  end

  always @(*) begin
    if(when_Stream_l1196) begin
      io_pop_payload_inst = readed_inst; // @[Stream.scala 1198:24]
    end else begin
      io_pop_payload_inst = io_push_payload_inst; // @[Stream.scala 1201:24]
    end
  end

  assign when_Stream_l1209 = (pushing != popping); // @[BaseType.scala 305:24]
  assign io_occupancy = (risingOccupancy && ptrMatch); // @[Stream.scala 1225:18]
  assign _zz_readed_error_1 = _zz_readed_error_2; // @[Phase.scala 647:23]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      risingOccupancy <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(when_Stream_l1209) begin
        risingOccupancy <= pushing; // @[Stream.scala 1210:21]
      end
      if(io_flush) begin
        risingOccupancy <= 1'b0; // @[Stream.scala 1237:21]
      end
    end
  end

  always @(posedge io_mainClk) begin
    if(when_Phase_l648) begin
      _zz_readed_error_2 <= {io_push_payload_inst,io_push_payload_error}; // @[Phase.scala 650:27]
    end
  end


endmodule

module UartCtrlRx (
  input      [2:0]    io_configFrame_dataLength,
  input      [0:0]    io_configFrame_stop,
  input      [1:0]    io_configFrame_parity,
  input               io_samplingTick,
  output              io_read_valid,
  input               io_read_ready,
  output     [7:0]    io_read_payload,
  input               io_rxd,
  output              io_rts,
  output reg          io_error,
  output              io_break,
  input               io_mainClk,
  input               resetCtrl_systemReset
);
  localparam UartStopType_ONE = 1'd0;
  localparam UartStopType_TWO = 1'd1;
  localparam UartParityType_NONE = 2'd0;
  localparam UartParityType_EVEN = 2'd1;
  localparam UartParityType_ODD = 2'd2;
  localparam UartCtrlRxState_IDLE = 3'd0;
  localparam UartCtrlRxState_START = 3'd1;
  localparam UartCtrlRxState_DATA = 3'd2;
  localparam UartCtrlRxState_PARITY = 3'd3;
  localparam UartCtrlRxState_STOP = 3'd4;

  wire                io_rxd_buffercc_io_dataOut;
  wire       [2:0]    _zz_when_UartCtrlRx_l139;
  wire       [0:0]    _zz_when_UartCtrlRx_l139_1;
  reg                 _zz_io_rts;
  wire                sampler_synchroniser;
  wire                sampler_samples_0;
  reg                 sampler_samples_1;
  reg                 sampler_samples_2;
  reg                 sampler_value;
  reg                 sampler_tick;
  reg        [2:0]    bitTimer_counter;
  reg                 bitTimer_tick;
  wire                when_UartCtrlRx_l43;
  reg        [2:0]    bitCounter_value;
  reg        [6:0]    break_counter;
  wire                break_valid;
  wire                when_UartCtrlRx_l69;
  reg        [2:0]    stateMachine_state;
  reg                 stateMachine_parity;
  reg        [7:0]    stateMachine_shifter;
  reg                 stateMachine_validReg;
  wire                when_UartCtrlRx_l93;
  wire                when_UartCtrlRx_l103;
  wire                when_UartCtrlRx_l111;
  wire                when_UartCtrlRx_l113;
  wire                when_UartCtrlRx_l125;
  wire                when_UartCtrlRx_l136;
  wire                when_UartCtrlRx_l139;
  `ifndef SYNTHESIS
  reg [23:0] io_configFrame_stop_string;
  reg [31:0] io_configFrame_parity_string;
  reg [47:0] stateMachine_state_string;
  `endif


  assign _zz_when_UartCtrlRx_l139_1 = ((io_configFrame_stop == UartStopType_ONE) ? 1'b0 : 1'b1);
  assign _zz_when_UartCtrlRx_l139 = {2'd0, _zz_when_UartCtrlRx_l139_1};
  BufferCC_3 io_rxd_buffercc (
    .io_dataIn             (io_rxd                    ), //i
    .io_dataOut            (io_rxd_buffercc_io_dataOut), //o
    .io_mainClk            (io_mainClk                ), //i
    .resetCtrl_systemReset (resetCtrl_systemReset     )  //i
  );
  `ifndef SYNTHESIS
  always @(*) begin
    case(io_configFrame_stop)
      UartStopType_ONE : io_configFrame_stop_string = "ONE";
      UartStopType_TWO : io_configFrame_stop_string = "TWO";
      default : io_configFrame_stop_string = "???";
    endcase
  end
  always @(*) begin
    case(io_configFrame_parity)
      UartParityType_NONE : io_configFrame_parity_string = "NONE";
      UartParityType_EVEN : io_configFrame_parity_string = "EVEN";
      UartParityType_ODD : io_configFrame_parity_string = "ODD ";
      default : io_configFrame_parity_string = "????";
    endcase
  end
  always @(*) begin
    case(stateMachine_state)
      UartCtrlRxState_IDLE : stateMachine_state_string = "IDLE  ";
      UartCtrlRxState_START : stateMachine_state_string = "START ";
      UartCtrlRxState_DATA : stateMachine_state_string = "DATA  ";
      UartCtrlRxState_PARITY : stateMachine_state_string = "PARITY";
      UartCtrlRxState_STOP : stateMachine_state_string = "STOP  ";
      default : stateMachine_state_string = "??????";
    endcase
  end
  `endif

  always @(*) begin
    io_error = 1'b0; // @[UartCtrlRx.scala 23:12]
    case(stateMachine_state)
      UartCtrlRxState_IDLE : begin
      end
      UartCtrlRxState_START : begin
      end
      UartCtrlRxState_DATA : begin
      end
      UartCtrlRxState_PARITY : begin
        if(bitTimer_tick) begin
          if(!when_UartCtrlRx_l125) begin
            io_error = 1'b1; // @[UartCtrlRx.scala 130:22]
          end
        end
      end
      default : begin
        if(bitTimer_tick) begin
          if(when_UartCtrlRx_l136) begin
            io_error = 1'b1; // @[UartCtrlRx.scala 137:22]
          end
        end
      end
    endcase
  end

  assign io_rts = _zz_io_rts; // @[UartCtrlRx.scala 24:10]
  assign sampler_synchroniser = io_rxd_buffercc_io_dataOut; // @[CrossClock.scala 13:9]
  assign sampler_samples_0 = sampler_synchroniser; // @[Utils.scala 1115:50]
  always @(*) begin
    bitTimer_tick = 1'b0; // @[UartCtrlRx.scala 40:16]
    if(sampler_tick) begin
      if(when_UartCtrlRx_l43) begin
        bitTimer_tick = 1'b1; // @[UartCtrlRx.scala 44:14]
      end
    end
  end

  assign when_UartCtrlRx_l43 = (bitTimer_counter == 3'b000); // @[BaseType.scala 305:24]
  assign break_valid = (break_counter == 7'h41); // @[BaseType.scala 305:24]
  assign when_UartCtrlRx_l69 = (io_samplingTick && (! break_valid)); // @[BaseType.scala 305:24]
  assign io_break = break_valid; // @[UartCtrlRx.scala 75:12]
  assign io_read_valid = stateMachine_validReg; // @[UartCtrlRx.scala 84:19]
  assign when_UartCtrlRx_l93 = ((sampler_tick && (! sampler_value)) && (! break_valid)); // @[BaseType.scala 305:24]
  assign when_UartCtrlRx_l103 = (sampler_value == 1'b1); // @[BaseType.scala 305:24]
  assign when_UartCtrlRx_l111 = (bitCounter_value == io_configFrame_dataLength); // @[BaseType.scala 305:24]
  assign when_UartCtrlRx_l113 = (io_configFrame_parity == UartParityType_NONE); // @[BaseType.scala 305:24]
  assign when_UartCtrlRx_l125 = (stateMachine_parity == sampler_value); // @[BaseType.scala 305:24]
  assign when_UartCtrlRx_l136 = (! sampler_value); // @[BaseType.scala 299:24]
  assign when_UartCtrlRx_l139 = (bitCounter_value == _zz_when_UartCtrlRx_l139); // @[BaseType.scala 305:24]
  assign io_read_payload = stateMachine_shifter; // @[UartCtrlRx.scala 146:19]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      _zz_io_rts <= 1'b0; // @[Data.scala 400:33]
      sampler_samples_1 <= 1'b1; // @[Data.scala 400:33]
      sampler_samples_2 <= 1'b1; // @[Data.scala 400:33]
      sampler_value <= 1'b1; // @[Data.scala 400:33]
      sampler_tick <= 1'b0; // @[Data.scala 400:33]
      break_counter <= 7'h0; // @[Data.scala 400:33]
      stateMachine_state <= UartCtrlRxState_IDLE; // @[Data.scala 400:33]
      stateMachine_validReg <= 1'b0; // @[Data.scala 400:33]
    end else begin
      _zz_io_rts <= (! io_read_ready); // @[Reg.scala 39:30]
      if(io_samplingTick) begin
        sampler_samples_1 <= sampler_samples_0; // @[Utils.scala 1109:24]
      end
      if(io_samplingTick) begin
        sampler_samples_2 <= sampler_samples_1; // @[Utils.scala 1109:24]
      end
      sampler_value <= (((1'b0 || ((1'b1 && sampler_samples_0) && sampler_samples_1)) || ((1'b1 && sampler_samples_0) && sampler_samples_2)) || ((1'b1 && sampler_samples_1) && sampler_samples_2)); // @[Reg.scala 39:30]
      sampler_tick <= io_samplingTick; // @[Reg.scala 39:30]
      if(sampler_value) begin
        break_counter <= 7'h0; // @[UartCtrlRx.scala 67:15]
      end else begin
        if(when_UartCtrlRx_l69) begin
          break_counter <= (break_counter + 7'h01); // @[UartCtrlRx.scala 70:17]
        end
      end
      stateMachine_validReg <= 1'b0; // @[Reg.scala 39:30]
      case(stateMachine_state)
        UartCtrlRxState_IDLE : begin
          if(when_UartCtrlRx_l93) begin
            stateMachine_state <= UartCtrlRxState_START; // @[Enum.scala 148:67]
          end
        end
        UartCtrlRxState_START : begin
          if(bitTimer_tick) begin
            stateMachine_state <= UartCtrlRxState_DATA; // @[Enum.scala 148:67]
            if(when_UartCtrlRx_l103) begin
              stateMachine_state <= UartCtrlRxState_IDLE; // @[Enum.scala 148:67]
            end
          end
        end
        UartCtrlRxState_DATA : begin
          if(bitTimer_tick) begin
            if(when_UartCtrlRx_l111) begin
              if(when_UartCtrlRx_l113) begin
                stateMachine_state <= UartCtrlRxState_STOP; // @[Enum.scala 148:67]
                stateMachine_validReg <= 1'b1; // @[UartCtrlRx.scala 115:24]
              end else begin
                stateMachine_state <= UartCtrlRxState_PARITY; // @[Enum.scala 148:67]
              end
            end
          end
        end
        UartCtrlRxState_PARITY : begin
          if(bitTimer_tick) begin
            if(when_UartCtrlRx_l125) begin
              stateMachine_state <= UartCtrlRxState_STOP; // @[Enum.scala 148:67]
              stateMachine_validReg <= 1'b1; // @[UartCtrlRx.scala 127:22]
            end else begin
              stateMachine_state <= UartCtrlRxState_IDLE; // @[Enum.scala 148:67]
            end
          end
        end
        default : begin
          if(bitTimer_tick) begin
            if(when_UartCtrlRx_l136) begin
              stateMachine_state <= UartCtrlRxState_IDLE; // @[Enum.scala 148:67]
            end else begin
              if(when_UartCtrlRx_l139) begin
                stateMachine_state <= UartCtrlRxState_IDLE; // @[Enum.scala 148:67]
              end
            end
          end
        end
      endcase
    end
  end

  always @(posedge io_mainClk) begin
    if(sampler_tick) begin
      bitTimer_counter <= (bitTimer_counter - 3'b001); // @[UartCtrlRx.scala 42:15]
      if(when_UartCtrlRx_l43) begin
        bitTimer_counter <= 3'b100; // @[UartCtrlRx.scala 46:19]
      end
    end
    if(bitTimer_tick) begin
      bitCounter_value <= (bitCounter_value + 3'b001); // @[UartCtrlRx.scala 58:13]
    end
    if(bitTimer_tick) begin
      stateMachine_parity <= (stateMachine_parity ^ sampler_value); // @[UartCtrlRx.scala 88:14]
    end
    case(stateMachine_state)
      UartCtrlRxState_IDLE : begin
        if(when_UartCtrlRx_l93) begin
          bitTimer_counter <= 3'b001; // @[UartCtrlRx.scala 39:27]
        end
      end
      UartCtrlRxState_START : begin
        if(bitTimer_tick) begin
          bitCounter_value <= 3'b000; // @[UartCtrlRx.scala 55:25]
          stateMachine_parity <= (io_configFrame_parity == UartParityType_ODD); // @[UartCtrlRx.scala 102:18]
        end
      end
      UartCtrlRxState_DATA : begin
        if(bitTimer_tick) begin
          stateMachine_shifter[bitCounter_value] <= sampler_value; // @[UartCtrlRx.scala 110:37]
          if(when_UartCtrlRx_l111) begin
            bitCounter_value <= 3'b000; // @[UartCtrlRx.scala 55:25]
          end
        end
      end
      UartCtrlRxState_PARITY : begin
        if(bitTimer_tick) begin
          bitCounter_value <= 3'b000; // @[UartCtrlRx.scala 55:25]
        end
      end
      default : begin
      end
    endcase
  end


endmodule

module UartCtrlTx (
  input      [2:0]    io_configFrame_dataLength,
  input      [0:0]    io_configFrame_stop,
  input      [1:0]    io_configFrame_parity,
  input               io_samplingTick,
  input               io_write_valid,
  output reg          io_write_ready,
  input      [7:0]    io_write_payload,
  input               io_cts,
  output              io_txd,
  input               io_break,
  input               io_mainClk,
  input               resetCtrl_systemReset
);
  localparam UartStopType_ONE = 1'd0;
  localparam UartStopType_TWO = 1'd1;
  localparam UartParityType_NONE = 2'd0;
  localparam UartParityType_EVEN = 2'd1;
  localparam UartParityType_ODD = 2'd2;
  localparam UartCtrlTxState_IDLE = 3'd0;
  localparam UartCtrlTxState_START = 3'd1;
  localparam UartCtrlTxState_DATA = 3'd2;
  localparam UartCtrlTxState_PARITY = 3'd3;
  localparam UartCtrlTxState_STOP = 3'd4;

  wire       [2:0]    _zz_clockDivider_counter_valueNext;
  wire       [0:0]    _zz_clockDivider_counter_valueNext_1;
  wire       [2:0]    _zz_when_UartCtrlTx_l93;
  wire       [0:0]    _zz_when_UartCtrlTx_l93_1;
  reg                 clockDivider_counter_willIncrement;
  wire                clockDivider_counter_willClear;
  reg        [2:0]    clockDivider_counter_valueNext;
  reg        [2:0]    clockDivider_counter_value;
  wire                clockDivider_counter_willOverflowIfInc;
  wire                clockDivider_counter_willOverflow;
  reg        [2:0]    tickCounter_value;
  reg        [2:0]    stateMachine_state;
  reg                 stateMachine_parity;
  reg                 stateMachine_txd;
  wire                when_UartCtrlTx_l58;
  wire                when_UartCtrlTx_l73;
  wire                when_UartCtrlTx_l76;
  wire                when_UartCtrlTx_l93;
  wire       [2:0]    _zz_stateMachine_state;
  reg                 _zz_io_txd;
  `ifndef SYNTHESIS
  reg [23:0] io_configFrame_stop_string;
  reg [31:0] io_configFrame_parity_string;
  reg [47:0] stateMachine_state_string;
  reg [47:0] _zz_stateMachine_state_string;
  `endif


  assign _zz_clockDivider_counter_valueNext_1 = clockDivider_counter_willIncrement;
  assign _zz_clockDivider_counter_valueNext = {2'd0, _zz_clockDivider_counter_valueNext_1};
  assign _zz_when_UartCtrlTx_l93_1 = ((io_configFrame_stop == UartStopType_ONE) ? 1'b0 : 1'b1);
  assign _zz_when_UartCtrlTx_l93 = {2'd0, _zz_when_UartCtrlTx_l93_1};
  `ifndef SYNTHESIS
  always @(*) begin
    case(io_configFrame_stop)
      UartStopType_ONE : io_configFrame_stop_string = "ONE";
      UartStopType_TWO : io_configFrame_stop_string = "TWO";
      default : io_configFrame_stop_string = "???";
    endcase
  end
  always @(*) begin
    case(io_configFrame_parity)
      UartParityType_NONE : io_configFrame_parity_string = "NONE";
      UartParityType_EVEN : io_configFrame_parity_string = "EVEN";
      UartParityType_ODD : io_configFrame_parity_string = "ODD ";
      default : io_configFrame_parity_string = "????";
    endcase
  end
  always @(*) begin
    case(stateMachine_state)
      UartCtrlTxState_IDLE : stateMachine_state_string = "IDLE  ";
      UartCtrlTxState_START : stateMachine_state_string = "START ";
      UartCtrlTxState_DATA : stateMachine_state_string = "DATA  ";
      UartCtrlTxState_PARITY : stateMachine_state_string = "PARITY";
      UartCtrlTxState_STOP : stateMachine_state_string = "STOP  ";
      default : stateMachine_state_string = "??????";
    endcase
  end
  always @(*) begin
    case(_zz_stateMachine_state)
      UartCtrlTxState_IDLE : _zz_stateMachine_state_string = "IDLE  ";
      UartCtrlTxState_START : _zz_stateMachine_state_string = "START ";
      UartCtrlTxState_DATA : _zz_stateMachine_state_string = "DATA  ";
      UartCtrlTxState_PARITY : _zz_stateMachine_state_string = "PARITY";
      UartCtrlTxState_STOP : _zz_stateMachine_state_string = "STOP  ";
      default : _zz_stateMachine_state_string = "??????";
    endcase
  end
  `endif

  always @(*) begin
    clockDivider_counter_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(io_samplingTick) begin
      clockDivider_counter_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  assign clockDivider_counter_willClear = 1'b0; // @[Utils.scala 537:19]
  assign clockDivider_counter_willOverflowIfInc = (clockDivider_counter_value == 3'b100); // @[BaseType.scala 305:24]
  assign clockDivider_counter_willOverflow = (clockDivider_counter_willOverflowIfInc && clockDivider_counter_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    if(clockDivider_counter_willOverflow) begin
      clockDivider_counter_valueNext = 3'b000; // @[Utils.scala 552:17]
    end else begin
      clockDivider_counter_valueNext = (clockDivider_counter_value + _zz_clockDivider_counter_valueNext); // @[Utils.scala 554:17]
    end
    if(clockDivider_counter_willClear) begin
      clockDivider_counter_valueNext = 3'b000; // @[Utils.scala 558:15]
    end
  end

  always @(*) begin
    stateMachine_txd = 1'b1; // @[UartCtrlTx.scala 49:15]
    case(stateMachine_state)
      UartCtrlTxState_IDLE : begin
      end
      UartCtrlTxState_START : begin
        stateMachine_txd = 1'b0; // @[UartCtrlTx.scala 63:13]
      end
      UartCtrlTxState_DATA : begin
        stateMachine_txd = io_write_payload[tickCounter_value]; // @[UartCtrlTx.scala 71:13]
      end
      UartCtrlTxState_PARITY : begin
        stateMachine_txd = stateMachine_parity; // @[UartCtrlTx.scala 85:13]
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    io_write_ready = io_break; // @[UartCtrlTx.scala 55:20]
    case(stateMachine_state)
      UartCtrlTxState_IDLE : begin
      end
      UartCtrlTxState_START : begin
      end
      UartCtrlTxState_DATA : begin
        if(clockDivider_counter_willOverflow) begin
          if(when_UartCtrlTx_l73) begin
            io_write_ready = 1'b1; // @[UartCtrlTx.scala 74:28]
          end
        end
      end
      UartCtrlTxState_PARITY : begin
      end
      default : begin
      end
    endcase
  end

  assign when_UartCtrlTx_l58 = ((io_write_valid && (! io_cts)) && clockDivider_counter_willOverflow); // @[BaseType.scala 305:24]
  assign when_UartCtrlTx_l73 = (tickCounter_value == io_configFrame_dataLength); // @[BaseType.scala 305:24]
  assign when_UartCtrlTx_l76 = (io_configFrame_parity == UartParityType_NONE); // @[BaseType.scala 305:24]
  assign when_UartCtrlTx_l93 = (tickCounter_value == _zz_when_UartCtrlTx_l93); // @[BaseType.scala 305:24]
  assign _zz_stateMachine_state = (io_write_valid ? UartCtrlTxState_START : UartCtrlTxState_IDLE); // @[Expression.scala 1420:25]
  assign io_txd = _zz_io_txd; // @[UartCtrlTx.scala 101:10]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      clockDivider_counter_value <= 3'b000; // @[Data.scala 400:33]
      stateMachine_state <= UartCtrlTxState_IDLE; // @[Data.scala 400:33]
      _zz_io_txd <= 1'b1; // @[Data.scala 400:33]
    end else begin
      clockDivider_counter_value <= clockDivider_counter_valueNext; // @[Reg.scala 39:30]
      case(stateMachine_state)
        UartCtrlTxState_IDLE : begin
          if(when_UartCtrlTx_l58) begin
            stateMachine_state <= UartCtrlTxState_START; // @[Enum.scala 148:67]
          end
        end
        UartCtrlTxState_START : begin
          if(clockDivider_counter_willOverflow) begin
            stateMachine_state <= UartCtrlTxState_DATA; // @[Enum.scala 148:67]
          end
        end
        UartCtrlTxState_DATA : begin
          if(clockDivider_counter_willOverflow) begin
            if(when_UartCtrlTx_l73) begin
              if(when_UartCtrlTx_l76) begin
                stateMachine_state <= UartCtrlTxState_STOP; // @[Enum.scala 148:67]
              end else begin
                stateMachine_state <= UartCtrlTxState_PARITY; // @[Enum.scala 148:67]
              end
            end
          end
        end
        UartCtrlTxState_PARITY : begin
          if(clockDivider_counter_willOverflow) begin
            stateMachine_state <= UartCtrlTxState_STOP; // @[Enum.scala 148:67]
          end
        end
        default : begin
          if(clockDivider_counter_willOverflow) begin
            if(when_UartCtrlTx_l93) begin
              stateMachine_state <= _zz_stateMachine_state; // @[UartCtrlTx.scala 94:19]
            end
          end
        end
      endcase
      _zz_io_txd <= (stateMachine_txd && (! io_break)); // @[Reg.scala 39:30]
    end
  end

  always @(posedge io_mainClk) begin
    if(clockDivider_counter_willOverflow) begin
      tickCounter_value <= (tickCounter_value + 3'b001); // @[UartCtrlTx.scala 40:13]
    end
    if(clockDivider_counter_willOverflow) begin
      stateMachine_parity <= (stateMachine_parity ^ stateMachine_txd); // @[UartCtrlTx.scala 52:14]
    end
    case(stateMachine_state)
      UartCtrlTxState_IDLE : begin
      end
      UartCtrlTxState_START : begin
        if(clockDivider_counter_willOverflow) begin
          stateMachine_parity <= (io_configFrame_parity == UartParityType_ODD); // @[UartCtrlTx.scala 66:18]
          tickCounter_value <= 3'b000; // @[UartCtrlTx.scala 37:25]
        end
      end
      UartCtrlTxState_DATA : begin
        if(clockDivider_counter_willOverflow) begin
          if(when_UartCtrlTx_l73) begin
            tickCounter_value <= 3'b000; // @[UartCtrlTx.scala 37:25]
          end
        end
      end
      UartCtrlTxState_PARITY : begin
        if(clockDivider_counter_willOverflow) begin
          tickCounter_value <= 3'b000; // @[UartCtrlTx.scala 37:25]
        end
      end
      default : begin
      end
    endcase
  end


endmodule

module BufferCC_2 (
  input               io_dataIn,
  output              io_dataOut,
  input               io_mainClk,
  input               resetCtrl_mainClkReset
);

  (* async_reg = "true" *) reg                 buffers_0;
  (* async_reg = "true" *) reg                 buffers_1;

  initial begin
  `ifndef SYNTHESIS
    buffers_0 = $urandom;
    buffers_1 = $urandom;
  `endif
  end

  assign io_dataOut = buffers_1; // @[CrossClock.scala 38:14]
  always @(posedge io_mainClk) begin
    buffers_0 <= io_dataIn; // @[CrossClock.scala 31:14]
    buffers_1 <= buffers_0; // @[CrossClock.scala 34:16]
  end


endmodule

module BufferCC_3 (
  input               io_dataIn,
  output              io_dataOut,
  input               io_mainClk,
  input               resetCtrl_systemReset
);

  (* async_reg = "true" *) reg                 buffers_0;
  (* async_reg = "true" *) reg                 buffers_1;

  assign io_dataOut = buffers_1; // @[CrossClock.scala 38:14]
  always @(posedge io_mainClk or posedge resetCtrl_systemReset) begin
    if(resetCtrl_systemReset) begin
      buffers_0 <= 1'b0; // @[Data.scala 400:33]
      buffers_1 <= 1'b0; // @[Data.scala 400:33]
    end else begin
      buffers_0 <= io_dataIn; // @[CrossClock.scala 31:14]
      buffers_1 <= buffers_0; // @[CrossClock.scala 34:16]
    end
  end


endmodule
