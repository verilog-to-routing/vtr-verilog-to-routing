#!/usr/bin/env python3
# generate a 2-dut systemverilog testbench. rtl vs post-synth (after abc).

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional

from port_parse import PortDecl, isClockPort, isResetPort


@dataclass(frozen=True)
class DirectedRamPlan:
    # port names used for directed same-addr ram collision stimulus
    kind: str  # "sp_bits" | "regfile_bits" | "dp_vector"
    note: str


def _portByLower(ports: List[PortDecl], lowerName: str) -> Optional[PortDecl]:
    for port in ports:
        if port.name.lower() == lowerName:
            return port
    return None


# USE: return a directed-ram plan when top ports look like known ram harnesses.
def detectDirectedRamPlan(ports: List[PortDecl]) -> Optional[DirectedRamPlan]:
    inputs = {p.name.lower(): p for p in ports if p.direction == "input"}
    if (
        "we1" in inputs
        and "we2" in inputs
        and "addr1" in inputs
        and "addr2" in inputs
        and "data1" in inputs
        and "data2" in inputs
    ):
        return DirectedRamPlan(
            kind="dp_vector",
            note="dual-port vector ports: same-addr read/write and write/write",
        )
    if (
        "we" in inputs
        and "waddr0" in inputs
        and "raddr0" in inputs
        and "wd0" in inputs
    ):
        return DirectedRamPlan(
            kind="regfile_bits",
            note="regfile-style dual-port: same-addr read+write (we1 hardwired 0 in rtl)",
        )
    if "we" in inputs and "a0" in inputs and "d0" in inputs:
        return DirectedRamPlan(
            kind="sp_bits",
            note="single-port bit-blasted: same-addr read+write",
        )
    return None


def _assignBits(prefix: str, width: int, value: int) -> List[str]:
    lines = []
    for bitIdx in range(width):
        bitVal = (value >> bitIdx) & 1
        lines.append(f"        {prefix}{bitIdx} = 1'b{bitVal};")
    return lines


# USE: emit directed vectors that hit same-cycle same-address ram collisions.
def _directedRamStimulusBody(
    ports: List[PortDecl],
    plan: DirectedRamPlan,
    primaryClk: str,
) -> str:
    compareChecks = []
    outputs = [p for p in ports if p.direction == "output"]
    for p in outputs:
        compareChecks.append(
            f"""            if ({p.name}_rtl !== {p.name}_synth) begin
                if (errors < MAX_ERRORS) begin
                    $display("MISMATCH directed port={p.name} rtl=%b synth=%b",
                             {p.name}_rtl, {p.name}_synth);
                end
                errors++;
            end"""
        )
    compareBody = "\n".join(compareChecks) if compareChecks else "            ;"
    tick = f"""
        @(posedge {primaryClk});
        #1;
{compareBody}
        if (errors >= MAX_ERRORS) begin
            $display("aborting after %0d errors (directed ram)", errors);
            $display("VCHECK_SUMMARY matched=0 mismatched=0 vectors=0 port_errors=%0d", errors);
            $fatal(1);
        end
"""
    if plan.kind == "sp_bits":
        body = "\n".join(
            [
                "        // directed sp_ram: write then same-addr read+write",
                "        we = 1'b1;",
                *_assignBits("a", 3, 0),
                *_assignBits("d", 1, 1),
                tick,
                "        we = 1'b1;",
                *_assignBits("a", 3, 0),
                *_assignBits("d", 1, 0),
                tick,
                "        we = 1'b0;",
                *_assignBits("a", 3, 0),
                tick,
            ]
        )
        return f"\n        $display(\"directed ram: {plan.note}\");\n{body}\n"
    if plan.kind == "regfile_bits":
        body = "\n".join(
            [
                "        // directed regfile: write then same-addr read+write",
                "        we = 1'b1;",
                *_assignBits("waddr", 5, 0),
                *_assignBits("raddr", 5, 0),
                *_assignBits("wd", 1, 1),
                tick,
                "        we = 1'b1;",
                *_assignBits("waddr", 5, 0),
                *_assignBits("raddr", 5, 0),
                *_assignBits("wd", 1, 0),
                tick,
                "        we = 1'b0;",
                *_assignBits("waddr", 5, 0),
                *_assignBits("raddr", 5, 0),
                tick,
            ]
        )
        return f"\n        $display(\"directed ram: {plan.note}\");\n{body}\n"
    we1 = _portByLower(ports, "we1")
    we2 = _portByLower(ports, "we2")
    addr1 = _portByLower(ports, "addr1")
    addr2 = _portByLower(ports, "addr2")
    data1 = _portByLower(ports, "data1")
    data2 = _portByLower(ports, "data2")
    if not all((we1, we2, addr1, addr2, data1, data2)):
        return ""
    aw = min(addr1.width, addr2.width, 8)
    dw = min(data1.width, data2.width, 8)
    body = "\n".join(
        [
            "        // directed dp_ram: port1 write, then same-addr read+write",
            "        we1 = 1'b1; we2 = 1'b0;",
            f"        addr1 = {aw}'d0; addr2 = {aw}'d0;",
            f"        data1 = {dw}'d1; data2 = {dw}'d0;",
            tick,
            "        we1 = 1'b1; we2 = 1'b1;",
            f"        addr1 = {aw}'d0; addr2 = {aw}'d0;",
            f"        data1 = {dw}'d2; data2 = {dw}'d3;",
            tick,
            "        we1 = 1'b0; we2 = 1'b0;",
            f"        addr1 = {aw}'d0; addr2 = {aw}'d0;",
            tick,
        ]
    )
    return f"\n        $display(\"directed ram: {plan.note}\");\n{body}\n"


def _isActiveLowReset(name: str) -> bool:
    # active-low when the name looks like *_n / n* / rstn / resetn
    lower = name.lower()
    return lower.endswith("_n") or lower.startswith("n") or "rstn" in lower or "resetn" in lower


def _isHoldHighBringupPort(name: str) -> bool:
    # bring-up / enable-style pins held high through reset so the dut leaves reset
    lower = name.lower()
    return any(
        token in lower
        for token in ("rdy", "ready", "enable", "en", "start", "go", "valid_in")
    )


def _logicDecl(name: str, width: int) -> str:
    if width <= 1:
        return f"    logic {name};"
    return f"    logic [{width - 1}:0] {name};"


def _portConnect(port: PortDecl, suffix: str) -> str:
    # inputs are shared; outputs are per-dut with _rtl / _synth suffixes
    if port.direction == "input":
        return f"        .{port.name}({port.name})"
    if port.direction == "output":
        return f"        .{port.name}({port.name}_{suffix})"
    return f"        .{port.name}({port.name})"


# USE: emit tb that drives identical random inputs into rtl and post-synth duts.
def generateDualTestbench(
    ports: List[PortDecl],
    *,
    rtlModule: str,
    synthModule: str,
    numVectors: int,
    seed: int,
    maxErrors: int = 20,
    directedRam: bool = False,
) -> str:
    clocks = [p for p in ports if p.direction == "input" and isClockPort(p.name)]
    resets = [p for p in ports if p.direction == "input" and isResetPort(p.name)]
    dataInputs = [
        p for p in ports
        if p.direction == "input" and not isClockPort(p.name) and not isResetPort(p.name)
    ]
    outputs = [p for p in ports if p.direction == "output"]
    combinatorial = len(clocks) == 0

    inputDecls = "\n".join(_logicDecl(p.name, p.width) for p in ports if p.direction == "input")
    outputDecls = "\n".join(
        _logicDecl(f"{p.name}_rtl", p.width) + "\n" + _logicDecl(f"{p.name}_synth", p.width)
        for p in outputs
    )

    rtlPorts = ",\n".join(_portConnect(p, "rtl") for p in ports)
    synthPorts = ",\n".join(_portConnect(p, "synth") for p in ports)

    # free-running clocks for sequential tops
    clockBlocks = []
    for clk in clocks:
        period = 10
        clockBlocks.append(
            f"""
    initial {clk.name} = 1'b0;
    always #{period // 2} {clk.name} = ~{clk.name};
"""
        )
    clockSv = "\n".join(clockBlocks)

    # reset polarity follows common *_n / rstn naming
    resetInit = []
    for rst in resets:
        activeLow = _isActiveLowReset(rst.name)
        resetInit.append(f"        {rst.name} = 1'b{'0' if activeLow else '1'};")
    resetDeassert = []
    for rst in resets:
        activeLow = _isActiveLowReset(rst.name)
        resetDeassert.append(f"        {rst.name} = 1'b{'1' if activeLow else '0'};")

    holdHighDuringReset = [p for p in dataInputs if _isHoldHighBringupPort(p.name)]
    holdHighInit = [f"        {p.name} = 1'b1;" for p in holdHighDuringReset]
    zeroDataInputs = [p for p in dataInputs if p not in holdHighDuringReset]

    # known values before reset/warm-up so x-propagation does not fake mismatches
    zeroLines = []
    for p in zeroDataInputs:
        if p.width <= 1:
            zeroLines.append(f"        {p.name} = 1'b0;")
        else:
            zeroLines.append(f"        {p.name} = '0;")
    zeroBody = "\n".join(zeroLines + holdHighInit) if (zeroLines or holdHighInit) else "        ;"

    # randomize non-clock / non-reset inputs each vector
    randLines = []
    for p in dataInputs:
        if p.width <= 1:
            randLines.append(f"            {p.name} = $urandom_range(0, 1);")
        elif p.width <= 32:
            randLines.append(f"            {p.name} = $urandom();")
        else:
            words = (p.width + 31) // 32
            parts = ["$urandom()" for _ in range(words)]
            joined = " , ".join(parts)
            randLines.append(f"            {p.name} = {{{joined}}};")
    randBody = "\n".join(randLines) if randLines else "            ;"

    # compare every output bit between rtl and post-synth
    compareChecks = []
    for p in outputs:
        compareChecks.append(
            f"""            if ({p.name}_rtl !== {p.name}_synth) begin
                if (errors < MAX_ERRORS) begin
                    $display("MISMATCH vec=%0d port={p.name} rtl=%b synth=%b",
                             vecIdx, {p.name}_rtl, {p.name}_synth);
                end
                errors++;
                vecFail = 1;
            end"""
        )
    compareBody = "\n".join(compareChecks) if compareChecks else "            ;"
    vectorTally = """
            if (vecFail) begin
                vecMismatched++;
            end else begin
                vecMatched++;
            end
"""
    abortSummary = """
                $display("VCHECK_SUMMARY matched=%0d mismatched=%0d vectors=%0d port_errors=%0d",
                         vecMatched, vecMismatched, vecMatched + vecMismatched, errors);
"""

    clockNames = ", ".join(p.name for p in clocks) if clocks else "(none)"
    resetNames = ", ".join(p.name for p in resets) if resets else "(none)"
    pinBanner = f"""
        $display("tb clocks: {clockNames}");
        $display("tb resets: {resetNames}");
"""

    directedBlock = ""
    if directedRam and not combinatorial:
        ramPlan = detectDirectedRamPlan(ports)
        if ramPlan is not None:
            primaryClk = clocks[0].name
            directedBlock = _directedRamStimulusBody(ports, ramPlan, primaryClk)
        else:
            directedBlock = """
        $display("directed ram: skipped (top ports do not match known ram harness shapes)");
"""

    if combinatorial:
        stimulus = f"""
        errors = 0;
        vecMatched = 0;
        vecMismatched = 0;
{pinBanner}
        for (vecIdx = 0; vecIdx < NUM_VECTORS; vecIdx++) begin
            vecFail = 0;
{randBody}
            #1;
{compareBody}{vectorTally}
            if (errors >= MAX_ERRORS) begin
                $display("aborting after %0d errors", errors);
{abortSummary}
                $fatal(1);
            end
        end
"""
    else:
        primaryClk = clocks[0].name if clocks else "clk"
        if resets:
            resetBlock = (
                zeroBody
                + "\n"
                + "\n".join(resetInit)
                + f"""
        // hold reset asserted while inputs are quiet
        repeat (8) @(posedge {primaryClk});
"""
                + "\n".join(resetDeassert)
                + f"""
        // settle after deassert before random stimulus
        repeat (4) @(posedge {primaryClk});
"""
            )
        else:
            resetBlock = zeroBody + f"""
        // no reset port on this top; zero inputs and warm up before random
        repeat (16) @(posedge {primaryClk});
"""
        stimulus = f"""
        errors = 0;
        vecMatched = 0;
        vecMismatched = 0;
{pinBanner}{resetBlock}{directedBlock}
        for (vecIdx = 0; vecIdx < NUM_VECTORS; vecIdx++) begin
            vecFail = 0;
{randBody}
            @(posedge {primaryClk});
            #1;
{compareBody}{vectorTally}
            if (errors >= MAX_ERRORS) begin
                $display("aborting after %0d errors", errors);
{abortSummary}
                $fatal(1);
            end
        end
"""

    return f"""`timescale 1ns/1ps
// auto-generated by mosaic/verilator_check/tb_generator.py
// compares rtl vs post-synth (after abc) under identical random stimulus.
// no $srandom (verilator pli); seed via +verilator+seed+N / $urandom.
module tb;
    localparam int NUM_VECTORS = {numVectors};
    localparam int MAX_ERRORS = {maxErrors};
    // seed hint for humans; verilator uses +verilator+seed+{seed}
    localparam int SEED_HINT = {seed};

{inputDecls}

{outputDecls}

    integer vecIdx;
    integer errors;
    integer vecMatched;
    integer vecMismatched;
    integer vecFail;

    {rtlModule} dut_rtl (
{rtlPorts}
    );

    {synthModule} dut_synth (
{synthPorts}
    );
{clockSv}
    initial begin
{stimulus}
        $display("VCHECK_SUMMARY matched=%0d mismatched=%0d vectors=%0d port_errors=%0d",
                 vecMatched, vecMismatched, vecMatched + vecMismatched, errors);
        if (errors == 0) begin
            $display("PASS: %0d vectors matched across rtl/post-synth", NUM_VECTORS);
            $finish;
        end else begin
            $display("FAIL: %0d mismatches in %0d vectors", errors, NUM_VECTORS);
            $fatal(1);
        end
    end
endmodule
"""
