`timescale 1ns / 1ps
/*
 * File         : CPZero.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0   16-Sep-2011  GEA       Initial design.
 *   2.0   14-May-2012  GEA       Complete rework.
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   The MIPS-32 Coprocessor 0 (CP0). This is the processor management unit that allows
 *   interrupts, traps, system calls, and other exceptions. It distinguishes
 *   user and kernel modes, provides status information, and can override program
 *   flow. This processor is designed for "bare metal" memory access and thus does
 *   not have virtual memory hardware as a part of it. However, the subset of CP0
 *   is MIPS-32-compliant.
 */
module CPZero(
    input  clock,
    //-- CP0 Functionality --//
    input  Mfc0,                    // CPU instruction is Mfc0
    input  Mtc0,                    // CPU instruction is Mtc0
    input  IF_Stall,
    input  ID_Stall,                // Commits are not made during stalls
    input  COP1,                    // Instruction for Coprocessor 1
    input  COP2,                    // Instruction for Coprocessor 2
    input  COP3,                    // Instruction for Coprocessor 3
    input  ERET,                    // Instruction is ERET (Exception Return)
    input  [4:0] Rd,                // Specifies Cp0 register
    input  [2:0] Sel,               // Specifies Cp0 'select'
    input  [31:0] Reg_In,           // Data from GP register to replace CP0 register
    output reg [31:0] Reg_Out,      // Data from CP0 register for GP register
    output KernelMode,              // Kernel mode indicator for pipeline transit
    output ReverseEndian,           // Reverse Endian memory indicator for User Mode
    //-- Hw Interrupts --//
    input  [4:0] Int,               // Five hardware interrupts external to the processor
    //-- Exceptions --//
    input  reset,                   // Cold Reset (EXC_Reset)
    input  EXC_NMI,                 // Non-Maskable Interrupt
    input  EXC_AdIF,                // Address Error Exception from i-fetch (mapped to AdEL)
    input  EXC_AdEL,                // Address Error Exception from data memory load
    input  EXC_AdES,                // Address Error Exception from data memory store
    input  EXC_Ov,                  // Integer Overflow Exception
    input  EXC_Tr,                  // Trap Exception
    input  EXC_Sys,                 // System Call Exception
    input  EXC_Bp,                  // Breakpoint Exception
    input  EXC_RI,                  // Reserved Instruction Exception
    //-- Exception Data --//
    input  [31:0] ID_RestartPC,     // PC for exception, whether PC of instruction or of branch (PC-4) if BDS
    input  [31:0] EX_RestartPC,     // Same as 'ID_RestartPC' but in EX stage
    input  [31:0] M_RestartPC,      // Same as 'ID_RestartPC' but in MEM stage
    input  ID_IsFlushed,
    input  IF_IsBD,                 // Indicator of IF exception being a branch delay slot instruction
    input  ID_IsBD,                 // Indicator of ID exception being a branch delay slot instruction
    input  EX_IsBD,                 // Indicator of EX exception being a branch delay slot instruction
    input  M_IsBD,                  // Indicator of M  exception being a branch delay slot instruction
    input  [31:0] BadAddr_M,        // Bad 'Virtual' Address for exceptions AdEL, AdES in MEM stage
    input  [31:0] BadAddr_IF,       // Bad 'Virtual' Address for AdIF (i.e. AdEL) in IF stage
    input  ID_CanErr,               // Cumulative signal, i.e. (ID_ID_CanErr | ID_EX_CanErr | ID_M_CanErr)
    input  EX_CanErr,               // Cumulative signal, i.e. (EX_EX_CanErr | EX_M_CanErr)
    input  M_CanErr,                // Memory stage can error (i.e. cause exception)
    //-- Exception Control Flow --/
    output IF_Exception_Stall,
    output ID_Exception_Stall,
    output EX_Exception_Stall,
    output M_Exception_Stall,
    output IF_Exception_Flush,
    output ID_Exception_Flush,
    output EX_Exception_Flush,
    output M_Exception_Flush,
    output Exc_PC_Sel,              // Mux selector for exception PC override
    output reg [31:0] Exc_PC_Out,   // Address for PC at the beginning of an exception
    output [7:0] IP                 // Pending Interrupts from Cause register (for diagnostic purposes)
    );

    `include "MIPS_Parameters.v"


    /***
     Exception Control Flow Notes

     - Exceptions can occur in every pipeline stage. This implies that more than one exception
       can be raised in a single cycle. When this occurs, only the forward-most exception
       (i.e. MEM over EX) is handled. This and the following note guarantee program order.

     - An exception in any pipeline stage must stall that stage until all following stages are
       exception-free. This is because it only makes sense for exceptions to occur in program order.

     - A pipeline stage which causes an exception must flush, i.e. prevent any commits it would
       have normally made and convert itself to a NOP for the next pipeline stage. Furthermore,
       it must flush all previous pipeline stages as well in order to retain program order.

     - Instructions reading CP0 (mtc0) read in ID without further action. Writes to CP0 (mtc0,
       eret) also write in ID, but only after forward pipeline stages have been cleared
       of possible exceptions. This prevents many insidious bugs, such as switching to User Mode
       in ID when a legitimate memory access in kernel mode is processing in MEM, or conversely
       a switch to Kernel Mode in ID when an instruction in User Mode is attempting a kernel region
       memory access (when a kernel mode signal does not propagate through the pipeline).

     - Commits occur in ID (CP0), EX (HILO), MEM, and WB (registers).

     - Hardware interrupts are detected and inserted in the ID stage, but only when there are no
       other possible exceptions in the pipeline. Because they appear 'asynchronous' to the
       processor, the remaining instructions in forward stages (EX, MEM, WB) can either be
       flushed or completed. It is simplest to have them complete to avoid restarts, but the
       interrupt latency is higher if e.g. the MEM stage stalls on a memory access (this would
       be unavoidable on single-cycle processors). This implementation allows all forward instructions
       to complete, for a greater instruction throughput but higher interrupt latency.

     - Software interrupts should appear synchronous in the program order, meaning that all
       instructions previous to them should complete and no instructions after them should start
       until the interrupts has been processed.

     Exception Name             Short Name          Pipeline Stage
       Address Error Ex         (AdEL, AdES)        MEM, IF
       Integer Overflow Ex      (Ov)                EX
       Trap Ex                  (Tr)                MEM
       Syscall                  (Sys)               ID
       Breakpoint               (Bp)                ID
       Reserved Instruction     (RI)                ID
       Coprocessor Unusable     (CpU)               ID
       Interrupt                (Int)               ID
       Reset, SReset, NMI                           ID
    ***/


    // Exceptions Generated Internally
    wire EXC_CpU;

    // Hardware Interrupt #5, caused by Timer/Perf counter
    wire Int5;

    // Top-level Authoritative Interrupt Signal
    wire EXC_Int;

    // General Exception detection (all but Interrupts, Reset, Soft Reset, and NMI)
    wire EXC_General = EXC_AdIF | EXC_AdEL | EXC_AdES | EXC_Ov | EXC_Tr | EXC_Sys | EXC_Bp | EXC_RI | EXC_CpU;

    // Misc
    wire CP0_WriteCond;
    reg  [3:0] Cause_ExcCode_bits;

    reg reset_r;
    always @(posedge clock) begin
        reset_r <= reset;
    end

    /***
     MIPS-32 COPROCESSOR 0 (Cp0) REGISTERS

     These are defined in "MIPS32 Architecture for Programmers Volume III:
     The MIPS32 Privileged Resource Architecture" from MIPS Technologies, Inc.

     Optional registers are omitted. Changes to the processor (such as adding
     an MMU/TLB, etc. must be reflected in these registers.
    */

    // BadVAddr Register (Register 8, Select 0)
    reg [31:0] BadVAddr;

    // Count Register (Register 9, Select 0)
    reg [31:0] Count;

    // Compare Register (Register 11, Select 0)
    reg [31:0] Compare;

    // Status Register (Register 12, Select 0)
    wire [2:0] Status_CU_321 = 3'b000;
    reg  Status_CU_0;       // Access Control to CPs, [2]->Cp3, ... [0]->Cp0
    wire Status_RP = 0;
    wire Status_FR = 0;
    reg  Status_RE;         // Reverse Endian Memory for User Mode
    wire Status_MX = 0;
    wire Status_PX = 0;
    reg  Status_BEV;        // Exception vector locations (0->Norm, 1->Bootstrap)
    wire Status_TS = 0;
    wire Status_SR = 0;     // Soft reset not implemented
    reg  Status_NMI;        // Non-Maskable Interrupt
    wire Status_RES = 0;
    wire [1:0] Status_Custom = 2'b00;
    reg [7:0] Status_IM;    // Interrupt mask
    wire Status_KX = 0;
    wire Status_SX = 0;
    wire Status_UX = 0;
    reg  Status_UM;         // Base operating mode (0->Kernel, 1->User)
    wire Status_R0 = 0;
    reg  Status_ERL;        // Error Level     (0->Normal, 1->Error (reset, NMI))
    reg  Status_EXL;        // Exception level (0->Normal, 1->Exception)
    reg  Status_IE;         // Interrupt Enable
    wire [31:0] Status = {Status_CU_321, Status_CU_0, Status_RP, Status_FR, Status_RE, Status_MX,
                                 Status_PX, Status_BEV, Status_TS, Status_SR, Status_NMI, Status_RES,
                                 Status_Custom, Status_IM, Status_KX, Status_SX, Status_UX,
                                 Status_UM, Status_R0, Status_ERL, Status_EXL, Status_IE};

    // Cause Register (Register 13, Select 0)
    reg  Cause_BD;                  // Exception occured in Branch Delay
    reg  [1:0] Cause_CE;            // CP number for CP Unusable exception
    reg  Cause_IV;                  // Indicator of general IV (0->0x180) or special IV (1->0x200)
    wire Cause_WP = 0;
    reg  [7:0] Cause_IP;            // Pending HW Interrupt indicator.
    wire Cause_ExcCode4 = 0;        // Can be made into a register when this bit is needed.
    reg  [3:0] Cause_ExcCode30;     // Description of Exception (only lower 4 bits currently used; see above)
    wire [31:0] Cause  = {Cause_BD, 1'b0, Cause_CE, 4'b0000, Cause_IV, Cause_WP,
                                 6'b000000, Cause_IP, 1'b0, Cause_ExcCode4, Cause_ExcCode30, 2'b00};

    // Exception Program Counter (Register 14, Select 0)
    reg [31:0] EPC;

    // Processor Identification (Register 15, Select 0)
    wire [7:0] ID_Options = 8'b0000_0000;
    wire [7:0] ID_CID = 8'b0000_0000;
    wire [7:0] ID_PID = 8'b0000_0000;
    wire [7:0] ID_Rev = 8'b0000_0001;
    wire [31:0] PRId = {ID_Options, ID_CID, ID_PID, ID_Rev};

    // Configuration Register (Register 16, Select 0)
    wire Config_M = 1;
    wire [14:0] Config_Impl = 15'b000_0000_0000_0000;
    wire Config_BE = `Big_Endian;
    wire [1:0] Config_AT = 2'b00;
    wire [2:0] Config_AR = 3'b000;
    wire [2:0] Config_MT = 3'b000;
    wire [2:0] Config_K0 = 3'b000;
    wire [31:0] Config = {Config_M, Config_Impl, Config_BE, Config_AT, Config_AR, Config_MT,
                                 4'b0000, Config_K0};

    // Configuration Register 1 (Register 16, Select 1)
    wire Config1_M = 0;
    wire [5:0] Config1_MMU = 6'b000000;
    wire [2:0] Config1_IS = 3'b000;
    wire [2:0] Config1_IL = 3'b000;
    wire [2:0] Config1_IA = 3'b000;
    wire [2:0] Config1_DS = 3'b000;
    wire [2:0] Config1_DL = 3'b000;
    wire [2:0] Config1_DA = 3'b000;
    wire Config1_C2 = 0;
    wire Config1_MD = 0;
    wire Config1_PC = 0;    // XXX Performance Counters
    wire Config1_WR = 0;    // XXX Watch Registers
    wire Config1_CA = 0;
    wire Config1_EP = 0;
    wire Config1_FP = 0;
    wire [31:0] Config1 = {Config1_M, Config1_MMU, Config1_IS, Config1_IL, Config1_IA,
                                  Config1_DS, Config1_DL, Config1_DA, Config1_C2,
                                  Config1_MD, Config1_PC, Config1_WR, Config1_CA,
                                  Config1_EP, Config1_FP};

    // Performance Counter Register (Register 25) XXX TODO

    // ErrorEPC Register (Register 30, Select 0)
    reg [31:0] ErrorEPC;

    // Exception Detection and Processing
    wire M_Exception_Detect, EX_Exception_Detect, ID_Exception_Detect, IF_Exception_Detect;
    wire M_Exception_Mask, EX_Exception_Mask, ID_Exception_Mask, IF_Exception_Mask;
    wire M_Exception_Ready, EX_Exception_Ready, ID_Exception_Ready, IF_Exception_Ready;

    assign IP = Cause_IP;

    /*** Coprocessor Unusable Exception ***/
    assign EXC_CpU = COP1 | COP2 | COP3 | ((Mtc0 | Mfc0 | ERET) & ~(Status_CU_0 | KernelMode));

    /*** Kernel Mode Signal ***/
    assign KernelMode = ~Status_UM | Status_EXL | Status_ERL;

    /*** Reverse Endian for User Mode ***/
    assign ReverseEndian = Status_RE;

    /*** Interrupts ***/
    assign Int5 = (Count == Compare);
    wire   Enabled_Interrupt = EXC_NMI | (Status_IE & ((Cause_IP[7:0] & Status_IM[7:0]) != 8'h00));
    assign EXC_Int = Enabled_Interrupt & ~Status_EXL & ~Status_ERL & ~ID_IsFlushed;

    assign CP0_WriteCond = (Status_CU_0 | KernelMode) & Mtc0 & ~ID_Stall;


    /***
     Exception Hazard Flow Control Explanation:
        - An exception at any time in any stage causes its own and any previous stages to
          flush (clear own commits, NOPS to fwd stages).
        - An exception in a stage can also stall that stage (and inherently all previous stages) if and only if:
            1. A forward stage is capable of causing an exception AND
            2. A forward stage is not currently causing an exception.
        - An exception is ready to process when it is detected and not stalled in a stage.

        Flush specifics per pipeline stage:
            MEM: Mask 'MemWrite' and 'MemRead' (for performance) after EX/M and before data memory. NOPs to M/WB.
            EX : Mask writes to HI/LO. NOPs to EX/M.
            ID : Mask writes (reads?) to CP0. NOPs to ID/EX.
            IF : NOP to IF/ID.
    ***/

    /*** Exceptions grouped by pipeline stage ***/
    assign   M_Exception_Detect = EXC_AdEL | EXC_AdES | EXC_Tr;
    assign  EX_Exception_Detect = EXC_Ov;
    assign  ID_Exception_Detect = EXC_Sys | EXC_Bp | EXC_RI | EXC_CpU | EXC_Int;
    assign  IF_Exception_Detect = EXC_AdIF;

    /*** Exception mask conditions ***/

    // A potential bug would occur if e.g. EX stalls, MEM has data, but MEM is not stalled and finishes
    // going through the pipeline so forwarding would fail. This is not a problem however because
    // EX would not need data since it would flush on an exception.
    assign   M_Exception_Mask = IF_Stall;
    assign  EX_Exception_Mask = IF_Stall | M_CanErr;
    assign  ID_Exception_Mask = IF_Stall | M_CanErr | EX_CanErr;
    assign  IF_Exception_Mask = M_CanErr | EX_CanErr | ID_CanErr | EXC_Int;

    /***
     Exceptions which must wait for forward stages. A stage will not stall if a forward stage has an exception.
     These stalls must be inserted as stall conditions in the hazard unit so that it will take care of chaining.
     All writes to CP0 must also wait for forward hazard conditions to clear.
    */
    assign  M_Exception_Stall  =  M_Exception_Detect  & M_Exception_Mask;
    assign  EX_Exception_Stall =  EX_Exception_Detect & EX_Exception_Mask & ~M_Exception_Detect;
    assign  ID_Exception_Stall = (ID_Exception_Detect | ERET | Mtc0) & ID_Exception_Mask & ~(EX_Exception_Detect | M_Exception_Detect);
    assign  IF_Exception_Stall =  IF_Exception_Detect & IF_Exception_Mask & ~(ID_Exception_Detect | EX_Exception_Detect | M_Exception_Detect);


    /*** Exceptions which are ready to process (mutually exclusive) ***/
    // XXX can remove ~ID_Stall since in mask now (?)
    assign   M_Exception_Ready =  ~ID_Stall & M_Exception_Detect  & ~M_Exception_Mask;
    assign  EX_Exception_Ready =  ~ID_Stall & EX_Exception_Detect & ~EX_Exception_Mask;
    assign  ID_Exception_Ready =  ~ID_Stall & ID_Exception_Detect & ~ID_Exception_Mask;
    assign  IF_Exception_Ready =  ~ID_Stall & IF_Exception_Detect & ~IF_Exception_Mask;

    /***
     Flushes. A flush clears a pipeline stage's control signals and prevents the stage from committing any changes.
     Data such as 'RestartPC' and the detected exception must remain.
    */
    assign   M_Exception_Flush = M_Exception_Detect;
    assign  EX_Exception_Flush = M_Exception_Detect | EX_Exception_Detect;
    assign  ID_Exception_Flush = M_Exception_Detect | EX_Exception_Detect | ID_Exception_Detect;
    assign  IF_Exception_Flush = M_Exception_Detect | EX_Exception_Detect | ID_Exception_Detect | IF_Exception_Detect | (ERET & ~ID_Stall) | reset_r;


    /*** Software reads of CP0 Registers ***/
    always @(*) begin
        if (Mfc0 & (Status_CU_0 | KernelMode)) begin
            case (Rd)
                5'd8  : Reg_Out <= BadVAddr;
                5'd9  : Reg_Out <= Count;
                5'd11 : Reg_Out <= Compare;
                5'd12 : Reg_Out <= Status;
                5'd13 : Reg_Out <= Cause;
                5'd14 : Reg_Out <= EPC;
                5'd15 : Reg_Out <= PRId;
                5'd16 : Reg_Out <= (Sel == 3'b000) ? Config : Config1;
                5'd30 : Reg_Out <= ErrorEPC;
                default : Reg_Out <= 32'h0000_0000;
            endcase
        end
        else begin
            Reg_Out <= 32'h0000_0000;
        end
    end

    /*** Cp0 Register Assignments: Non-general exceptions (Reset, Soft Reset, NMI...) ***/
    always @(posedge clock) begin
        if (reset) begin
            Status_BEV <= 1;
            Status_NMI <= 0;
            Status_ERL <= 1;
            ErrorEPC   <= 32'b0;
        end
        else if (ID_Exception_Ready & EXC_NMI) begin
            Status_BEV <= 1;
            Status_NMI <= 1;
            Status_ERL <= 1;
            ErrorEPC   <= ID_RestartPC;
        end
        else begin
            Status_BEV    <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[22]   : Status_BEV;
            Status_NMI    <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[19]   : Status_NMI;
            Status_ERL    <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[2]    : ((Status_ERL & ERET & ~ID_Stall) ? 1'b0 : Status_ERL);
            ErrorEPC      <= (CP0_WriteCond & (Rd == 5'd30) & (Sel == 3'b000)) ? Reg_In       : ErrorEPC;
        end
    end

    /*** Cp0 Register Assignments: All other registers ***/
    always @(posedge clock) begin
        if (reset) begin
            Count         <= 32'b0;
            Compare       <= 32'b0;
            Status_CU_0   <= 0;
            Status_RE     <= 0;
            Status_IM     <= 8'b0;
            Status_UM     <= 0;
            Status_IE     <= 0;
            Cause_IV      <= 0;
            Cause_IP      <= 8'b0;
        end
        else begin
            Count         <= (CP0_WriteCond & (Rd == 5'd9 ) & (Sel == 3'b000)) ? Reg_In       : Count + 1;
            Compare       <= (CP0_WriteCond & (Rd == 5'd11) & (Sel == 3'b000)) ? Reg_In       : Compare;
            Status_CU_0   <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[28]   : Status_CU_0;
            Status_RE     <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[25]   : Status_RE;
            Status_IM     <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[15:8] : Status_IM;
            Status_UM     <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[4]    : Status_UM;
            Status_IE     <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[0]    : Status_IE;
            Cause_IV      <= (CP0_WriteCond & (Rd == 5'd13) & (Sel == 3'b000)) ? Reg_In[23]   : Cause_IV;
            /* Cause_IP indicates 8 interrupts:
               [7]   is set by the timer comparison (Compare == Count), and cleared by writing to Compare.
               [6:2] are set and cleared by external hardware.
               [1:0] are set and cleared by software.
             */
            Cause_IP[7]   <= (CP0_WriteCond & (Rd == 5'd11) & (Sel == 3'b000)) ? 1'b0 : ((Int5) ? 1'b1 : Cause_IP[7]);
            Cause_IP[6:2] <= Int[4:0];
            Cause_IP[1:0] <= (CP0_WriteCond & (Rd == 5'd13) & (Sel == 3'b000)) ? Reg_In[9:8]  : Cause_IP[1:0];
        end
    end

    /*** Cp0 Register Assignments: General Exception and Interrupt Processing ***/
    always @(posedge clock) begin
        if (reset) begin
            Cause_BD <= 0;
            Cause_CE <= 2'b00;
            Cause_ExcCode30 <= 4'b0000;
            Status_EXL <= 0;
            EPC <= 32'h0;
            BadVAddr <= 32'h0;
        end
        else begin
            // MEM stage
            if (M_Exception_Ready) begin
                Cause_BD <= (Status_EXL) ? Cause_BD : M_IsBD;
                Cause_CE <= (COP3) ? 2'b11 : ((COP2) ? 2'b10 : ((COP1) ? 2'b01 : 2'b00));
                Cause_ExcCode30 <= Cause_ExcCode_bits;
                Status_EXL <= 1;
                EPC <= (Status_EXL) ? EPC : M_RestartPC;
                BadVAddr <= BadAddr_M;
            end
            // EX stage
            else if (EX_Exception_Ready) begin
                Cause_BD <= (Status_EXL) ? Cause_BD : EX_IsBD;
                Cause_CE <= (COP3) ? 2'b11 : ((COP2) ? 2'b10 : ((COP1) ? 2'b01 : 2'b00));
                Cause_ExcCode30 <= Cause_ExcCode_bits;
                Status_EXL <= 1;
                EPC <= (Status_EXL) ? EPC : EX_RestartPC;
                BadVAddr <= BadVAddr;
            end
            // ID stage
            else if (ID_Exception_Ready) begin
                Cause_BD <= (Status_EXL) ? Cause_BD : ID_IsBD;
                Cause_CE <= (COP3) ? 2'b11 : ((COP2) ? 2'b10 : ((COP1) ? 2'b01 : 2'b00));
                Cause_ExcCode30 <= Cause_ExcCode_bits;
                Status_EXL <= 1;
                EPC <= (Status_EXL) ? EPC : ID_RestartPC;
                BadVAddr <= BadVAddr;
            end
            // IF stage
            else if (IF_Exception_Ready) begin
                Cause_BD <= (Status_EXL) ? Cause_BD : IF_IsBD;
                Cause_CE <= (COP3) ? 2'b11 : ((COP2) ? 2'b10 : ((COP1) ? 2'b01 : 2'b00));
                Cause_ExcCode30 <= Cause_ExcCode_bits;
                Status_EXL <= 1;
                EPC <= (Status_EXL) ? EPC : BadAddr_IF;
                BadVAddr <= BadAddr_IF;
            end
            // No exceptions this cycle
            else begin
                Cause_BD <= 1'b0;
                Cause_CE <= Cause_CE;
                Cause_ExcCode30 <= Cause_ExcCode30;
                // Without new exceptions, 'Status_EXL' is set by software or cleared by ERET.
                Status_EXL <= (CP0_WriteCond & (Rd == 5'd12) & (Sel == 3'b000)) ? Reg_In[1] : ((Status_EXL & ERET & ~ID_Stall) ? 1'b0 : Status_EXL);
                // The EPC is also writable by software
                EPC <= (CP0_WriteCond & (Rd == 5'd14) & (Sel == 3'b000)) ? Reg_In : EPC;
                BadVAddr <= BadVAddr;
            end
        end
    end


    /*** Program Counter for all Exceptions/Interrupts ***/
    always @(*) begin
        // Following is redundant since PC has initial value now.
        if (reset) begin
            Exc_PC_Out <= `EXC_Vector_Base_Reset;
        end
        else if (ERET & ~ID_Stall) begin
            Exc_PC_Out <= (Status_ERL) ? ErrorEPC : EPC;
        end
        else if (EXC_General) begin
            Exc_PC_Out <= (Status_BEV) ? (`EXC_Vector_Base_Other_Boot   + `EXC_Vector_Offset_General) :
                                         (`EXC_Vector_Base_Other_NoBoot + `EXC_Vector_Offset_General);
        end
        else if (EXC_NMI) begin
            Exc_PC_Out <= `EXC_Vector_Base_Reset;
        end
        else if (EXC_Int & Cause_IV) begin
            Exc_PC_Out <= (Status_BEV) ? (`EXC_Vector_Base_Other_Boot   + `EXC_Vector_Offset_Special) :
                                         (`EXC_Vector_Base_Other_NoBoot + `EXC_Vector_Offset_Special);
        end
        else begin
            Exc_PC_Out <= (Status_BEV) ? (`EXC_Vector_Base_Other_Boot   + `EXC_Vector_Offset_General) :
                                         (`EXC_Vector_Base_Other_NoBoot + `EXC_Vector_Offset_General);
        end
    end

    //assign Exc_PC_Sel = (reset | (ERET & ~ID_Stall) | EXC_General | EXC_Int);
    assign Exc_PC_Sel = reset | (ERET & ~ID_Stall) | IF_Exception_Ready | ID_Exception_Ready | EX_Exception_Ready | M_Exception_Ready;

    /*** Cause Register ExcCode Field ***/
    always @(*) begin
        // Ordered by Pipeline Stage with Interrupts last
        if      (EXC_AdEL) Cause_ExcCode_bits <= 4'h4;     // 00100
        else if (EXC_AdES) Cause_ExcCode_bits <= 4'h5;     // 00101
        else if (EXC_Tr)   Cause_ExcCode_bits <= 4'hd;     // 01101
        else if (EXC_Ov)   Cause_ExcCode_bits <= 4'hc;     // 01100
        else if (EXC_Sys)  Cause_ExcCode_bits <= 4'h8;     // 01000
        else if (EXC_Bp)   Cause_ExcCode_bits <= 4'h9;     // 01001
        else if (EXC_RI)   Cause_ExcCode_bits <= 4'ha;     // 01010
        else if (EXC_CpU)  Cause_ExcCode_bits <= 4'hb;     // 01011
        else if (EXC_AdIF) Cause_ExcCode_bits <= 4'h4;     // 00100
        else if (EXC_Int)  Cause_ExcCode_bits <= 4'h0;     // 00000     // OK that NMI writes this.
        else               Cause_ExcCode_bits <= 4'bxxxx;
    end

endmodule

