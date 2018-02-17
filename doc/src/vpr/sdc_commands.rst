.. _sdc_commands:

SDC Commands
============
The following subset of SDC syntax is supported by VPR:

create_clock
------------
Creates a netlist or virtual clock.

Assigns a desired period (in nanoseconds) and waveform to one or more clocks in the netlist (if the ``–name`` option is omitted) or to a single virtual clock (used to constrain input and outputs to a clock external to the design).
Netlist clocks can be referred to using regular expressions, while the virtual clock name is taken as-is.

*Example Usage:*

.. code-block:: tcl

    #Create a netlist clock
    create_clock -period <float> <netlist clock list or regexes>

    #Create a virtual clock
    create_clock -period <float> -name <virtual clock name>

    #Create a netlist clock with custom waveform/duty-cycle
    create_clock -period <float> -waveform {rising_edge falling_edge} <netlist clock list or regexes>


Omitting the waveform creates a clock with a rising edge at 0 and a falling edge at the half period, and is equivalent to using ``-waveform {0 <period/2>}``.
Non-50% duty cycles are supported but behave no differently than 50% duty cycles, since falling edges are not used in analysis.
If a virtual clock is assigned using a create_clock command, it must be referenced elsewhere in a set_input_delay or set_output_delay constraint.

.. sdc:command:: create_clock

    .. sdc:option:: -period <float>

        Specifies the clock period.

        **Required:** Yes

    .. sdc:option:: -waveform {<float> <float>}

        Overrides the default clock waveform.

        The first value indicates the time the clock rises, the second the time the clock falls.

        **Required:** No

        **Default:** 50% duty cycle (i.e. ``-waveform {0 <period/2>}``).

    .. sdc:option:: -name <string>

        Creates a virtual clock with the specified name.

        **Required:** No

    .. sdc:option:: <netlist clock list or regexes>

        Creates a netlist clock

        **Required:** No

.. note:: One of :sdc:option:`-name` or :sdc:option:`<netlist clock list or regexes>` must be specified.

.. warning:: If a netlist clock is not specified with a :sdc:command:`create_clock` command, paths to and from that clock domain will not be analysed.


set_clock_groups
----------------
Specifies the relationship between groups of clocks.
May be used with netlist or virtual clocks in any combination.

Since VPR supports only the :sdc:option:`-exclusive` option, a :sdc:command:`set_clock_groups` constraint is equivalent to a :sdc:command:`set_false_path` constraint (see below) between each clock in one group and each clock in another.

For example, the following sets of commands are equivalent:

.. code-block:: tcl

    #Do not analyze any timing paths between clk and clk2, or between
    #clk and clk3
    set_clock_groups -exclusive -group {clk} -group {clk2 clk3}

and

.. code-block:: tcl

     set_false_path -from [get_clocks {clk}] -to [get_clocks {clk2 clk3}]
     set_false_path -from [get_clocks {clk2 clk3}] -to [get_clocks {clk}]

.. sdc:command:: set_clock_groups

    .. sdc:option:: -exclusive

        Indicates that paths between clock groups should not be analyzed.

        **Required:** Yes

        .. note:: VPR currently only supports exclusive clock groups


    .. sdc:option:: -group {<clock list or regexes>}

        Specifies a group of clocks.

        .. note:: At least 2 groups must be specified.

        **Required:** Yes


set_false_path
--------------
Cuts timing paths unidirectionally from each clock in :sdc:option:`-from` to each clock in :sdc:option:`–to`.
Otherwise equivalent to :sdc:command:`set_clock_groups`.

*Example Usage:*

.. code-block:: tcl

    #Do not analyze paths launched from clk and captured by clk2 or clk3
    set_false_path -from [get_clocks {clk}] -to [get_clocks {clk2 clk3}]

    #Do not analyze paths launched from clk2 or clk3 and captured by clk
    set_false_path -from [get_clocks {clk2 clk3}] -to [get_clocks {clk}]

.. note:: False paths are supported between entire clock domains, but *not* between individual registers.

.. sdc:command:: set_false_path

    .. sdc:option:: -from [get_clocks <clock list or regexes>]

        Specifies the source clock domain(s).

        **Required:** No

        **Default:** All clocks

    .. sdc:option:: -to [get_clocks <clock list or regexes>]

        Specifies the sink clock domain(s).

        **Required:** No

        **Default:** All clocks

set_max_delay/set_min_delay
---------------------------
Overrides the default setup (max) or hold (min) timing constraint calculated using the information from :sdc:command:`create_clock` with a user-specified delay.

*Example Usage:*

.. code-block:: tcl

    #Specify a maximum delay of 17 from input_clk to output_clk
    set_max_delay 17 -from [get_clocks {input_clk}] -to [get_clocks {output_clk}]

    #Specify a minimum delay of 2 from input_clk to output_clk
    set_min_delay 2 -from [get_clocks {input_clk}] -to [get_clocks {output_clk}]

.. note:: Max/Min delays are supported between entire clock domains, but *not* between individual netlist elements.

.. sdc:command:: set_max_delay/set_min_delay

    .. sdc:option:: <delay>

        The delay value to apply.

        **Required:** Yes

    .. sdc:option:: -from [get_clocks <clock list or regexes>]

        Specifies the source clock domain(s).

        **Required:** No

        **Default:** All clocks

    .. sdc:option:: -to [get_clocks <clock list or regexes>]

        Specifies the sink clock domain(s).

        **Required:** No

        **Default:** All clocks

set_multicycle_path
-------------------
Sets how many clock cycles elapse between the launch and capture edges for setup and hold checks.

The default the setup mutlicycle value is 1 (i.e. the capture setup check is performed against the edge one cycle after the launch edge).

The default hold multicycle is one less than the setup multicycle path (e.g. the capture hold check occurs in the same cycle as the launch edge for the default setup multicycle).

*Example Usage:*

.. code-block:: tcl

    #Create a 4 cycle setup check, and 3 cycle hold check from clkA to clkB
    set_multicycle_path -from [get_clocks {clkA}] -to [get_clocks {clkB}] 4

    #Create a 3 cycle setup check from clk to clk2
    # Note that this moves the default hold check to be 2 cycles
    set_multicycle_path -setup -from [get_clocks {clk}] -to [get_clocks {clk2}] 3

    #Create a 0 cycle hold check from clk to clk2
    # Note that this moves the default hold check back to it's original
    # position before the previous setup setup_multicycle_path was applied
    set_multicycle_path -hold -from [get_clocks {clk}] -to [get_clocks {clk2}] 2

.. note:: Multicycles are supported between entire clock domains, but *not* between individual registers.

.. sdc:command:: set_multicycle_path

    .. sdc:option:: -setup

        Indicates that the multicycle-path applies to setup analysis.

        **Required:** No

    .. sdc:option:: -hold

        Indicates that the multicycle-path applies to hold analysis.

        **Required:** No

    .. sdc:option:: -from [get_clocks <clock list or regexes>]

        Specifies the source clock domain(s).

        **Required:** No

        **Default:** All clocks

    .. sdc:option:: -to [get_clocks <clock list or regexes>]

        Specifies the sink clock domain(s).

        **Required:** No

        **Default:** All clocks

    .. sdc:option:: <path_multiplier>

        The number of cycles that apply to the specified path(s).

        **Required:** Yes

.. note:: If neither :sdc:option:`-setup` nor :sdc:option:`-hold` the setup multicycle is set to ``path_multiplier`` and the hold multicycle offset to ``0``.


set_input_delay/set_output_delay
--------------------------------
Use ``set_input_delay`` if you want timing paths from input I/Os analyzed, and ``set_output_delay`` if you want timing paths to output I/Os analyzed.

.. note:: If these commands are not specified in your SDC, paths from and to I/Os will not be timing analyzed.

These commands constrain each I/O pad specified after ``get_ports`` to be timing-equivalent to a register clocked on the clock specified after ``-clock``.
This can be either a clock signal in your design or a virtual clock that does not exist in the design but which is used only to specify the timing of I/Os.

The specified delays are added to I/O timing paths and can be used to model board level delays.

For single-clock circuits, ``-clock`` can be wildcarded using ``*`` to refer to the single netlist clock, although this is not supported in standard SDC.
This allows a single SDC command to constrain I/Os in all single-clock circuits.

*Example Usage:*

.. code-block:: tcl

    #Set a maximum input delay of 0.5 (relative to input_clk) on
    #ports in1, in2 and in3
    set_input_delay -clock input_clk -max 0.5 [get_ports {in1 in2 in3}]

    #Set a minimum output delay of 1.0 (relative to output_clk) on
    #all ports matching starting with 'out*'
    set_output_delay -clock output_clk -min 1 [get_ports {out*}]

    #Set both the maximum and minimum output delay to 0.3 for all I/Os
    #in the design
    set_output_delay -clock clk2 0.3 [get_ports {*}]

.. sdc:command:: set_input_delay/set_output_delay

    .. sdc:option:: -clock <virtual or netlist clock>

        Specifies the virtual or netlist clock the delay is relative to.

        **Required:** Yes

    .. sdc:option:: -max

        Specifies that the delay value should be treated as the maximum delay.

        **Required:** No

    .. sdc:option:: -min

        Specifies that the delay value should be treated as the minimum delay.

        **Required:** No

    .. sdc:option:: <delay>

        Specifies the delay value to be applied

        **Required:** Yes

    .. sdc:option:: [get_ports {<I/O list or regexes>}]

        Specifies the port names or port name regex.

        **Required:** Yes

    .. note::

        If neither ``-min`` nor ``-max`` are specified the delay value is applied to both.

set_clock_uncertainty
---------------------
Sets the clock uncertainty between clock domains.
This is typically used to model uncertainty in the clock arrival times due to clock jitter.

*Example Usage:*

.. code-block:: tcl

    #Sets the clock uncertainty between all clock domain pairs to 0.025
    set_clock_uncertainty 0.025

    #Sets the clock uncertainty from 'clk' to all other clock domains to 0.05
    set_clock_uncertainty -from [get_clocks {clk}] 0.05

    #Sets the clock uncertainty from 'clk' to 'clk2' to 0.75
    set_clock_uncertainty -from [get_clocks {clk}]  -to [get_clocks {clk2}] 0.75

.. sdc:command:: set_clock_uncertainty

    .. sdc:option:: -from [get_clocks <clock list or regexes>]

        Specifies the source clock domain(s).

        **Required:** No

        **Default:** All clocks

    .. sdc:option:: -to [get_clocks <clock list or regexes>]

        Specifies the sink clock domain(s).

        **Required:** No

        **Default:** All clocks

    .. sdc:option:: -setup

        Specifies the clock uncertainty for setup analysis.

        **Required:** No

    .. sdc:option:: -hold

        Specifies the clock uncertainty for hold analysis.

        **Required:** No

    .. sdc:option:: <uncertainty>

        The clock uncertainty value between the from and to clocks.

        **Required:** Yes

    .. note::

        If neither ``-setup`` nor ``-hold`` are specified the uncertainty value is applied to both.

set_clock_latency
-----------------
Sets the latency of a clock.
VPR automatically calculates on-chip clock network delay, and so only source latency is supported.

Source clock latency corresponds to the delay from the true clock source (e.g. off-chip clock generator) to the on-chip clock definition point.

.. code-block:: tcl

    #Sets the source clock latency of 'clk' to 1.0
    set_clock_latency -source 1.0 [get_clocks {clk}]

.. sdc:command:: set_clock_latency

    .. sdc:option:: -source

        Specifies that the latency is the source latency.

        **Required:** Yes

    .. sdc:option:: -early

        Specifies that the latency applies to early paths.

        **Required:** No

    .. sdc:option:: -late

        Specifies that the latency applies to late paths.

        **Required:** No

    .. sdc:option:: <latency>

        The clock's latency.

        **Required:** Yes

    .. sdc:option:: [get_clocks <clock list or regexes>]

        Specifies the clock domain(s).

        **Required:** Yes

    .. note::

        If neither ``-early`` nor ``-late`` are specified the latency value is applied to both.

set_disable_timing
------------------
Disables timing between a pair of connected pins in the netlist.
This is typically used to manually break combinational loops.

.. code-block:: tcl

    #Disables the timing edge between the pins 'in[0]' and 'out[0]' on
    #the netlist primitive named 'blk1'
    set_disable_timing -from [get_pins {blk1.in[0]}] -to [get_pins {blk1.out[0]}]

.. sdc:command:: set_disable_timing

    .. sdc:option:: -from [get_pins <pin list or regexes>]

        Specifies the source netlist pins.

        **Required:** Yes

    .. sdc:option:: -to [get_pins <pin list or regexes>]

        Specifies the sink netlist pins.

        **Required:** Yes


Special Characters
------------------
.. sdc:command:: # (comment), \\ (line continued), * (wildcard), {} (string escape)

    ``#`` starts a comment – everything remaining on this line will be ignored.

    ``\`` at the end of a line indicates that a command wraps to the next line.

    ``*`` is used in a ``get_clocks``/``get_ports`` command or at the end of ``create_clock`` to match all netlist clocks.
    Partial wildcarding (e.g. ``clk*`` to match ``clk`` and ``clk2``) is also supported.
    As mentioned above, ``*`` can be used in set_input_delay and set_output delay to refer to the netlist clock for single-clock circuits only, although this is not supported in standard SDC.

    ``{}`` escapes strings, e.g. ``{top^clk}`` matches a clock called ``top^clk``, while ``top^clk`` without braces gives an error because of the special ``^`` character.



.. _sdc_examples:

SDC Examples
-----------------
The following are sample SDC files for common non-default cases (assuming netlist clock domains ``clk`` and ``clk2``).


.. _sdc_example_A:

A
~~
Cut I/Os and analyse only register-to-register paths, including paths between clock domains; optimize to run as fast as possible.

.. code-block:: tcl

    create_clock -period 0 *


.. _sdc_example_B:

B
~~
Same as :ref:`sdc_example_A`, but with paths between clock domains cut.  Separate target frequencies are specified.

.. code-block:: tcl

    create_clock -period 2 clk
    create_clock -period 3 clk2
    set_clock_groups -exclusive -group {clk} -group {clk2}


.. _sdc_example_C:

C
~~
Same as :ref:`sdc_example_B`, but with paths to and from I/Os now analyzed.
This is the same as the multi-clock default, but with custom period constraints.

.. code-block:: tcl

    create_clock -period 2 clk
    create_clock -period 3 clk2
    create_clock -period 3.5 -name virtual_io_clock
    set_clock_groups -exclusive -group {clk} -group {clk2}
    set_input_delay -clock virtual_io_clock -max 0 [get_ports {*}]
    set_output_delay -clock virtual_io_clock -max 0 [get_ports {*}]



.. _sdc_example_D:

D
~~
Changing the phase between clocks, and accounting for delay through I/Os with set_input/output delay constraints.

.. code-block:: tcl

    #Custom waveform rising edge at 1.25, falling at 2.75
    create_clock -period 3 -waveform {1.25 2.75} clk
    create_clock -period 2 clk2
    create_clock -period 2.5 -name virtual_io_clock
    set_input_delay -clock virtual_io_clock -max 1 [get_ports {*}]
    set_output_delay -clock virtual_io_clock -max 0.5 [get_ports {*}]


.. _sdc_example_E:

E
~~
Sample using many supported SDC commands.  Inputs and outputs are constrained on separate virtual clocks.

.. code-block:: tcl

    create_clock -period 3 -waveform {1.25 2.75} clk
    create_clock -period 2 clk2
    create_clock -period 1 -name input_clk
    create_clock -period 0 -name output_clk
    set_clock_groups -exclusive -group input_clk -group clk2
    set_false_path -from [get_clocks {clk}] -to [get_clocks {output_clk}]
    set_max_delay 17 -from [get_clocks {input_clk}] -to [get_clocks {output_clk}]
    set_multicycle_path -setup -from [get_clocks {clk}] -to [get_clocks {clk2}] 3
    set_input_delay -clock input_clk -max 0.5 [get_ports {in1 in2 in3}]
    set_output_delay -clock output_clk -max 1 [get_ports {out*}]

