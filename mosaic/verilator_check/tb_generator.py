#!/usr/bin/env python3
"""generate a 3-dut systemverilog testbench. rtl vs synth vs abc."""

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
            f"""            if ({p.name}_rtl !== {p.name}_synth || {p.name}_rtl !== {p.name}_abc) begin
                if (errors < MAX_ERRORS) begin
                    $display("MISMATCH directed port={p.name} rtl=%b synth=%b abc=%b",
                             {p.name}_rtl, {p.name}_synth, {p.name}_abc);
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
            $fatal(1);
        end
"""
    if plan.kind == "sp_bits":
        body = "\n".join(
            [
                "        // directed sp_ram: write then same-addr read+write",
                "        we = 1'b1;",
                *_assignBits("a", 3, 0),
                *_assignBits("d", 8, 0x55),
            ]
        )
        body2 = "\n".join(
            [
                "        we = 1'b1;",
                *_assignBits("a", 3, 0),
                *_assignBits("d", 8, 0xAA),
            ]
        )
        body3 = "\n".join(
            [
                "        we = 1'b0;",
                *_assignBits("a", 3, 0),
                *_assignBits("d", 8, 0),
            ]
        )
        return f"""
        $display("directed ram: {plan.note}");
{body}
{tick}
{body2}
{tick}
{body3}
{tick}
"""
    if plan.kind == "regfile_bits":
        body = "\n".join(
            [
                "        // directed regfile: seed write",
                "        we = 1'b1;",
                *_assignBits("waddr", 2, 0),
                *_assignBits("raddr", 2, 1),
                *_assignBits("wd", 8, 0x3C),
            ]
        )
        body2 = "\n".join(
            [
                "        // same-addr read+write (raddr==waddr)",
                "        we = 1'b1;",
                *_assignBits("waddr", 2, 0),
                *_assignBits("raddr", 2, 0),
                *_assignBits("wd", 8, 0xC3),
            ]
        )
        body3 = "\n".join(
            [
                "        we = 1'b0;",
                *_assignBits("waddr", 2, 0),
                *_assignBits("raddr", 2, 0),
                *_assignBits("wd", 8, 0),
            ]
        )
        return f"""
        $display("directed ram: {plan.note}");
{body}
{tick}
{body2}
{tick}
{body3}
{tick}
"""
    # dp_vector is a full dual-port with both write enables
    data1 = _portByLower(ports, "data1")
    data2 = _portByLower(ports, "data2")
    assert data1 and data2
    return f"""
        $display("directed ram: {plan.note}");
        // seed via port2
        we1 = 1'b0;
        we2 = 1'b1;
        addr1 = '0;
        addr2 = '0;
        data1 = '0;
        data2 = {data2.width}'h55;
{tick}
        // same-addr read+write on port2 (read-first)
        we1 = 1'b0;
        we2 = 1'b1;
        addr1 = '0;
        addr2 = '0;
        data1 = '0;
        data2 = {data2.width}'hAA;
{tick}
        // same-addr write/write: port2 wins per sim_hardblocks.v policy
        we1 = 1'b1;
        we2 = 1'b1;
        addr1 = '0;
        addr2 = '0;
        data1 = {data1.width}'h11;
        data2 = {data2.width}'h22;
{tick}
        we1 = 1'b0;
        we2 = 1'b0;
        addr1 = '0;
        addr2 = '0;
        data1 = '0;
        data2 = '0;
{tick}
"""


def _isActiveLowReset(name: str) -> bool:
    lower = name.lower()
    return (
        lower.endswith("_n")
        or lower.startswith("nreset")
        or lower.startswith("nrst")
        or lower in {"nreset", "nrst", "reset_n", "rst_n"}
    )


# HELPER: pins that must be high for the dut to leave stall / idle during reset.
# arm_core's i_system_rdy stalls fetch when low. holding it high during the
# quiet reset/warm-up window avoids comparing while the core is frozen with
# undriven inputs.
def _isHoldHighBringupPort(name: str) -> bool:
    lower = name.lower()
    return lower in {"i_system_rdy", "system_rdy"}


def _logicDecl(name: str, width: int) -> str:
    if width <= 1:
        return f"    logic {name};"
    return f"    logic [{width - 1}:0] {name};"


def _portConnect(port: PortDecl, suffix: str) -> str:
    if port.direction == "input":
        return f"        .{port.name}({port.name})"
    if port.direction == "output":
        return f"        .{port.name}({port.name}_{suffix})"
    return f"        .{port.name}({port.name})"


# USE: emit tb that drives identical random inputs into three duts and compares outputs.
def generateTripleTestbench(
    ports: List[PortDecl],
    *,
    rtlModule: str,
    synthModule: str,
    abcModule: str,
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
        _logicDecl(f"{p.name}_rtl", p.width) + "\n" +
        _logicDecl(f"{p.name}_synth", p.width) + "\n" +
        _logicDecl(f"{p.name}_abc", p.width)
        for p in outputs
    )

    rtlPorts = ",\n".join(_portConnect(p, "rtl") for p in ports)
    synthPorts = ",\n".join(_portConnect(p, "synth") for p in ports)
    abcPorts = ",\n".join(_portConnect(p, "abc") for p in ports)

    # clock generators
    clockBlocks = []
    for clk in clocks:
        period = 10
        clockBlocks.append(f"""
    initial {clk.name} = 1'b0;
    always #{period // 2} {clk.name} = ~{clk.name};
""")
    clockSv = "\n".join(clockBlocks)

    # reset. assert active-high for names without _n. active-low for *_n / n*
    resetInit = []
    for rst in resets:
        activeLow = _isActiveLowReset(rst.name)
        resetInit.append(f"        {rst.name} = 1'b{'0' if activeLow else '1'};")
    resetDeassert = []
    for rst in resets:
        activeLow = _isActiveLowReset(rst.name)
        resetDeassert.append(f"        {rst.name} = 1'b{'1' if activeLow else '0'};")

    # hold known bring-up / stall pins quiet during reset so the circuit
    # actually comes out of reset instead of sitting stalled (e.g. arm_core
    # i_system_rdy). these are still randomized with the data inputs later.
    holdHighDuringReset = [
        p for p in dataInputs if _isHoldHighBringupPort(p.name)
    ]
    holdHighInit = [f"        {p.name} = 1'b1;" for p in holdHighDuringReset]
    zeroDataInputs = [
        p for p in dataInputs if p not in holdHighDuringReset
    ]

    # drive every non-clock/non-reset input to a known value before the
    # reset/warm-up window. leaving them undriven lets rtl and synth diverge
    # on x-propagation even when the netlists are functionally equivalent.
    zeroLines = []
    for p in zeroDataInputs:
        if p.width <= 1:
            zeroLines.append(f"        {p.name} = 1'b0;")
        else:
            zeroLines.append(f"        {p.name} = '0;")
    zeroBody = "\n".join(zeroLines + holdHighInit) if (zeroLines or holdHighInit) else "        ;"

    # randomize data inputs
    randLines = []
    for p in dataInputs:
        if p.width <= 1:
            randLines.append(f"            {p.name} = $urandom_range(0, 1);")
        elif p.width <= 32:
            randLines.append(f"            {p.name} = $urandom();")
        else:
            # pack multiple urandom words for wide buses
            words = (p.width + 31) // 32
            parts = []
            for w in range(words):
                parts.append("$urandom()")
            # truncate packed words to the declared width
            joined = " , ".join(parts)
            randLines.append(f"            {p.name} = {{{joined}}};")
    randBody = "\n".join(randLines) if randLines else "            ;"

    # compare outputs across the three duts
    compareChecks = []
    for p in outputs:
        compareChecks.append(
            f"""            if ({p.name}_rtl !== {p.name}_synth || {p.name}_rtl !== {p.name}_abc) begin
                if (errors < MAX_ERRORS) begin
                    $display("MISMATCH vec=%0d port={p.name} rtl=%b synth=%b abc=%b",
                             vecIdx, {p.name}_rtl, {p.name}_synth, {p.name}_abc);
                end
                errors++;
            end"""
        )
    compareBody = "\n".join(compareChecks) if compareChecks else "            ;"

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
{pinBanner}
        for (vecIdx = 0; vecIdx < NUM_VECTORS; vecIdx++) begin
{randBody}
            #1;
{compareBody}
            if (errors >= MAX_ERRORS) begin
                $display("aborting after %0d errors", errors);
                $fatal(1);
            end
        end
"""
    else:
        primaryClk = clocks[0].name if clocks else "clk"
        resetBlock = ""
        if resets:
            resetBlock = zeroBody + "\n" + "\n".join(resetInit) + f"""
        // hold reset asserted while inputs are quiet
        repeat (8) @(posedge {primaryClk});
""" + "\n".join(resetDeassert) + f"""
        // settle after deassert before random stimulus
        repeat (4) @(posedge {primaryClk});
"""
        else:
            resetBlock = zeroBody + f"""
        // no reset port on this top; zero inputs and warm up before random
        repeat (16) @(posedge {primaryClk});
"""
        stimulus = f"""
        errors = 0;
{pinBanner}{resetBlock}{directedBlock}
        for (vecIdx = 0; vecIdx < NUM_VECTORS; vecIdx++) begin
{randBody}
            @(posedge {primaryClk});
            #1;
{compareBody}
            if (errors >= MAX_ERRORS) begin
                $display("aborting after %0d errors", errors);
                $fatal(1);
            end
        end
"""

    return f"""`timescale 1ns/1ps
// auto-generated by mosaic/verilator_check/tb_generator.py
// compares rtl vs post-synth vs post-vtr-abc under identical random stimulus.
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

    {rtlModule} dut_rtl (
{rtlPorts}
    );

    {synthModule} dut_synth (
{synthPorts}
    );

    {abcModule} dut_abc (
{abcPorts}
    );
{clockSv}
    initial begin
{stimulus}
        if (errors == 0) begin
            $display("PASS: %0d vectors matched across rtl/synth/abc", NUM_VECTORS);
            $finish;
        end else begin
            $display("FAIL: %0d mismatches in %0d vectors", errors, NUM_VECTORS);
            $fatal(1);
        end
    end
endmodule
"""
