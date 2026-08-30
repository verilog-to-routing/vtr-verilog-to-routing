#!/usr/bin/env python3
"""generate a 2-dut systemverilog testbench comparing rtl vs post-synth (after abc)."""

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional

from port_parse import PortDecl, is_clock_port, is_reset_port


@dataclass(frozen=True)
class DirectedRamPlan:
    """port names used for directed same-addr ram collision stimulus."""

    kind: str  # "sp_bits" | "regfile_bits" | "dp_vector"
    note: str


def _port_by_lower(ports: List[PortDecl], lower_name: str) -> Optional[PortDecl]:
    """return the first port whose lowercased name matches lower_name."""
    for port in ports:
        if port.name.lower() == lower_name:
            return port
    return None


def detect_directed_ram_plan(  # pylint: disable=too-many-boolean-expressions
    ports: List[PortDecl],
) -> Optional[DirectedRamPlan]:
    """return a directed-ram plan when top ports look like known ram harnesses."""
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


def _assign_bits(prefix: str, width: int, value: int) -> List[str]:
    """return a list of SV assignment lines for individual bits of a bus."""
    lines = []
    for bit_idx in range(width):
        bit_val = (value >> bit_idx) & 1
        lines.append(f"        {prefix}{bit_idx} = 1'b{bit_val};")
    return lines


def _directed_ram_stimulus_body(  # pylint: disable=too-many-locals
    ports: List[PortDecl],
    plan: DirectedRamPlan,
    primary_clk: str,
) -> str:
    """emit directed vectors that hit same-cycle same-address ram collisions."""
    compare_checks = []
    outputs = [p for p in ports if p.direction == "output"]
    for p in outputs:
        compare_checks.append(
            f"""            if ({p.name}_rtl !== {p.name}_synth) begin
                if (errors < MAX_ERRORS) begin
                    $display("MISMATCH directed port={p.name} rtl=%b synth=%b",
                             {p.name}_rtl, {p.name}_synth);
                end
                errors++;
            end"""
        )
    compare_body = "\n".join(compare_checks) if compare_checks else "            ;"
    tick = f"""
        @(posedge {primary_clk});
        #1;
{compare_body}
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
                *_assign_bits("a", 3, 0),
                *_assign_bits("d", 1, 1),
                tick,
                "        we = 1'b1;",
                *_assign_bits("a", 3, 0),
                *_assign_bits("d", 1, 0),
                tick,
                "        we = 1'b0;",
                *_assign_bits("a", 3, 0),
                tick,
            ]
        )
        return f"\n        $display(\"directed ram: {plan.note}\");\n{body}\n"
    if plan.kind == "regfile_bits":
        body = "\n".join(
            [
                "        // directed regfile: write then same-addr read+write",
                "        we = 1'b1;",
                *_assign_bits("waddr", 5, 0),
                *_assign_bits("raddr", 5, 0),
                *_assign_bits("wd", 1, 1),
                tick,
                "        we = 1'b1;",
                *_assign_bits("waddr", 5, 0),
                *_assign_bits("raddr", 5, 0),
                *_assign_bits("wd", 1, 0),
                tick,
                "        we = 1'b0;",
                *_assign_bits("waddr", 5, 0),
                *_assign_bits("raddr", 5, 0),
                tick,
            ]
        )
        return f"\n        $display(\"directed ram: {plan.note}\");\n{body}\n"
    we1 = _port_by_lower(ports, "we1")
    we2 = _port_by_lower(ports, "we2")
    addr1 = _port_by_lower(ports, "addr1")
    addr2 = _port_by_lower(ports, "addr2")
    data1 = _port_by_lower(ports, "data1")
    data2 = _port_by_lower(ports, "data2")
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


def _is_active_low_reset(name: str) -> bool:
    """return True when the name looks like an active-low reset (*_n / n* / rstn / resetn)."""
    lower = name.lower()
    return lower.endswith("_n") or lower.startswith("n") or "rstn" in lower or "resetn" in lower


def _is_hold_high_bringup_port(name: str) -> bool:
    """return True for bring-up / enable-style pins held high through reset."""
    lower = name.lower()
    return any(
        token in lower
        for token in ("rdy", "ready", "enable", "en", "start", "go", "valid_in")
    )


def _logic_decl(name: str, width: int) -> str:
    """return a SV logic declaration for the given name and width."""
    if width <= 1:
        return f"    logic {name};"
    return f"    logic [{width - 1}:0] {name};"


def _port_connect(port: PortDecl, suffix: str) -> str:
    """return a SV port connection; inputs shared, outputs per-dut with suffix."""
    if port.direction == "input":
        return f"        .{port.name}({port.name})"
    if port.direction == "output":
        return f"        .{port.name}({port.name}_{suffix})"
    return f"        .{port.name}({port.name})"


def generate_dual_testbench(  # pylint: disable=too-many-arguments,too-many-locals,too-many-branches,too-many-statements
    ports: List[PortDecl],
    *,
    rtl_module: str,
    synth_module: str,
    num_vectors: int,
    seed: int,
    max_errors: int = 20,
    directed_ram: bool = False,
) -> str:
    """emit a tb that drives identical random inputs into rtl and post-synth duts."""
    clocks = [p for p in ports if p.direction == "input" and is_clock_port(p.name)]
    resets = [p for p in ports if p.direction == "input" and is_reset_port(p.name)]
    data_inputs = [
        p for p in ports
        if p.direction == "input" and not is_clock_port(p.name) and not is_reset_port(p.name)
    ]
    outputs = [p for p in ports if p.direction == "output"]
    combinatorial = len(clocks) == 0

    input_decls = "\n".join(_logic_decl(p.name, p.width) for p in ports if p.direction == "input")
    output_decls = "\n".join(
        _logic_decl(f"{p.name}_rtl", p.width) + "\n" + _logic_decl(f"{p.name}_synth", p.width)
        for p in outputs
    )

    rtl_ports = ",\n".join(_port_connect(p, "rtl") for p in ports)
    synth_ports = ",\n".join(_port_connect(p, "synth") for p in ports)

    # free-running clocks for sequential tops
    clock_blocks = []
    for clk in clocks:
        period = 10
        clock_blocks.append(
            f"""
    initial {clk.name} = 1'b0;
    always #{period // 2} {clk.name} = ~{clk.name};
"""
        )
    clock_sv = "\n".join(clock_blocks)

    # reset polarity follows common *_n / rstn naming
    reset_init = []
    for rst in resets:
        active_low = _is_active_low_reset(rst.name)
        reset_init.append(f"        {rst.name} = 1'b{'0' if active_low else '1'};")
    reset_deassert = []
    for rst in resets:
        active_low = _is_active_low_reset(rst.name)
        reset_deassert.append(f"        {rst.name} = 1'b{'1' if active_low else '0'};")

    hold_high_during_reset = [p for p in data_inputs if _is_hold_high_bringup_port(p.name)]
    hold_high_init = [f"        {p.name} = 1'b1;" for p in hold_high_during_reset]
    zero_data_inputs = [p for p in data_inputs if p not in hold_high_during_reset]

    # known values before reset/warm-up so x-propagation does not fake mismatches
    zero_lines = []
    for p in zero_data_inputs:
        if p.width <= 1:
            zero_lines.append(f"        {p.name} = 1'b0;")
        else:
            zero_lines.append(f"        {p.name} = '0;")
    zero_body = (
        "\n".join(zero_lines + hold_high_init)
        if (zero_lines or hold_high_init) else "        ;"
    )

    # randomize non-clock / non-reset inputs each vector
    rand_lines = []
    for p in data_inputs:
        if p.width <= 1:
            rand_lines.append(f"            {p.name} = $urandom_range(0, 1);")
        elif p.width <= 32:
            rand_lines.append(f"            {p.name} = $urandom();")
        else:
            words = (p.width + 31) // 32
            parts = ["$urandom()" for _ in range(words)]
            joined = " , ".join(parts)
            rand_lines.append(f"            {p.name} = {{{joined}}};")
    rand_body = "\n".join(rand_lines) if rand_lines else "            ;"

    # compare every output bit between rtl and post-synth
    compare_checks = []
    for p in outputs:
        compare_checks.append(
            f"""            if ({p.name}_rtl !== {p.name}_synth) begin
                if (errors < MAX_ERRORS) begin
                    $display("MISMATCH vec=%0d port={p.name} rtl=%b synth=%b",
                             vecIdx, {p.name}_rtl, {p.name}_synth);
                end
                errors++;
                vecFail = 1;
            end"""
        )
    compare_body = "\n".join(compare_checks) if compare_checks else "            ;"
    vector_tally = """
            if (vecFail) begin
                vecMismatched++;
            end else begin
                vecMatched++;
            end
"""
    abort_summary = """
                $display("VCHECK_SUMMARY matched=%0d mismatched=%0d vectors=%0d port_errors=%0d",
                         vecMatched, vecMismatched, vecMatched + vecMismatched, errors);
"""

    clock_names = ", ".join(p.name for p in clocks) if clocks else "(none)"
    reset_names = ", ".join(p.name for p in resets) if resets else "(none)"
    pin_banner = f"""
        $display("tb clocks: {clock_names}");
        $display("tb resets: {reset_names}");
"""

    directed_block = ""
    if directed_ram and not combinatorial:
        ram_plan = detect_directed_ram_plan(ports)
        if ram_plan is not None:
            primary_clk = clocks[0].name
            directed_block = _directed_ram_stimulus_body(ports, ram_plan, primary_clk)
        else:
            directed_block = """
        $display("directed ram: skipped (top ports do not match known ram harness shapes)");
"""

    if combinatorial:
        stimulus = f"""
        errors = 0;
        vecMatched = 0;
        vecMismatched = 0;
{pin_banner}
        for (vecIdx = 0; vecIdx < NUM_VECTORS; vecIdx++) begin
            vecFail = 0;
{rand_body}
            #1;
{compare_body}{vector_tally}
            if (errors >= MAX_ERRORS) begin
                $display("aborting after %0d errors", errors);
{abort_summary}
                $fatal(1);
            end
        end
"""
    else:
        primary_clk = clocks[0].name if clocks else "clk"
        if resets:
            reset_block = (
                zero_body
                + "\n"
                + "\n".join(reset_init)
                + f"""
        // hold reset asserted while inputs are quiet
        repeat (8) @(posedge {primary_clk});
"""
                + "\n".join(reset_deassert)
                + f"""
        // settle after deassert before random stimulus
        repeat (4) @(posedge {primary_clk});
"""
            )
        else:
            reset_block = zero_body + f"""
        // no reset port on this top; zero inputs and warm up before random
        repeat (16) @(posedge {primary_clk});
"""
        stimulus = f"""
        errors = 0;
        vecMatched = 0;
        vecMismatched = 0;
{pin_banner}{reset_block}{directed_block}
        for (vecIdx = 0; vecIdx < NUM_VECTORS; vecIdx++) begin
            vecFail = 0;
{rand_body}
            @(posedge {primary_clk});
            #1;
{compare_body}{vector_tally}
            if (errors >= MAX_ERRORS) begin
                $display("aborting after %0d errors", errors);
{abort_summary}
                $fatal(1);
            end
        end
"""

    return f"""`timescale 1ns/1ps
// auto-generated by mosaic/verilator_check/tb_generator.py
// compares rtl vs post-synth (after abc) under identical random stimulus.
// no $srandom (verilator pli); seed via +verilator+seed+N / $urandom.
module tb;
    localparam int NUM_VECTORS = {num_vectors};
    localparam int MAX_ERRORS = {max_errors};
    // seed hint for humans; verilator uses +verilator+seed+{seed}
    localparam int SEED_HINT = {seed};

{input_decls}

{output_decls}

    integer vecIdx;
    integer errors;
    integer vecMatched;
    integer vecMismatched;
    integer vecFail;

    {rtl_module} dut_rtl (
{rtl_ports}
    );

    {synth_module} dut_synth (
{synth_ports}
    );
{clock_sv}
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
