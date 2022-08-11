.. _arch_model_timing_tutorial:

Primitive Block Timing Modeling Tutorial
----------------------------------------
To accurately model an FPGA, the architect needs to specify the timing characteristics of the FPGA's primitives blocks.
This involves two key steps:

 #. Specifying the logical timing characteristics of a primitive including:

     * whether primitive pins are sequential or combinational, and
     * what the timing dependencies are between the pins.

 #. Specifying the physical delay values

These two steps separate the logical timing characteristics of a primitive, from the physically dependant delays.
This enables a single logical netlist primitive type (e.g. Flip-Flop) to be mapped into different physical locations with different timing characteristics.

The :ref:`FPGA architecture description <fpga_architecture_description>` describes the logical timing characteristics in the :ref:`models section <arch_models>`, while the physical timing information is specified on ``pb_types`` within :ref:`complex block <arch_complex_blocks>`.

The following sections illustrate some common block timing modeling approaches.


Combinational block
~~~~~~~~~~~~~~~~~~~
A typical combinational block is a full adder,

.. figure:: fa.*
    :width: 50%

    Full Adder

where ``a``, ``b`` and ``cin`` are combinational inputs, and ``sum`` and ``cout`` are combinational outputs.

We can model these timing dependencies on the model with the ``combinational_sink_ports``, which specifies the output ports which are dependant on an input port:

.. code-block:: xml

      <model name="adder">
        <input_ports>
          <port name="a" combinational_sink_ports="sum cout"/>
          <port name="b" combinational_sink_ports="sum cout"/>
          <port name="cin" combinational_sink_ports="sum cout"/>
        </input_ports>
        <output_ports>
          <port name="sum"/>
          <port name="cout"/>
        </output_ports>
      </model>

The physical timing delays are specified on any ``pb_type`` instances of the adder model.
For example:

.. code-block:: xml

    <pb_type name="adder" blif_model=".subckt adder" num_pb="1">
      <input name="a" num_pins="1"/>
      <input name="b" num_pins="1"/>
      <input name="cin" num_pins="1"/>
      <output name="cout" num_pins="1"/>
      <output name="sum" num_pins="1"/>

      <delay_constant max="300e-12" in_port="adder.a" out_port="adder.sum"/>
      <delay_constant max="300e-12" in_port="adder.b" out_port="adder.sum"/>
      <delay_constant max="300e-12" in_port="adder.cin" out_port="adder.sum"/>
      <delay_constant max="300e-12" in_port="adder.a" out_port="adder.cout"/>
      <delay_constant max="300e-12" in_port="adder.b" out_port="adder.cout"/>
      <delay_constant max="10e-12" in_port="adder.cin" out_port="adder.cout"/>
    </pb_type>

specifies that all the edges of 300ps delays, except to ``cin`` to ``cout`` edge which has a delay of 10ps.

.. _dff_timing_modeling:

Sequential block (no internal paths)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


A typical sequential block is a D-Flip-Flop (DFF).
DFFs have no internal timing paths between their input and output ports.

.. note:: If you are using BLIF's ``.latch`` directive to represent DFFs there is no need to explicitly provide a ``<model>`` definition, as it is supported by default.

.. figure:: dff.*
    :width: 50%

    DFF

Sequential model ports are specified by providing the ``clock="<name>"`` attribute, where ``<name>`` is the name of the associated clock ports.
The assoicated clock port must have ``is_clock="1"`` specified to indicate it is a clock.

.. code-block:: xml

      <model name="dff">
        <input_ports>
          <port name="d" clock="clk"/>
          <port name="clk" is_clock="1"/>
        </input_ports>
        <output_ports>
          <port name="q" clock="clk"/>
        </output_ports>
      </model>

The physical timing delays are specified on any ``pb_type`` instances of the model.
In the example below the setup-time of the input is specified as 66ps, while the clock-to-q delay of the output is set to 124ps.

.. code-block:: xml

    <pb_type name="ff" blif_model=".subckt dff" num_pb="1">
      <input name="D" num_pins="1"/>
      <output name="Q" num_pins="1"/>
      <clock name="clk" num_pins="1"/>

      <T_setup value="66e-12" port="ff.D" clock="clk"/>
      <T_clock_to_Q max="124e-12" port="ff.Q" clock="clk"/>
    </pb_type>


.. _mixed_sp_ram_timing_modeling:

Mixed Sequential/Combinational Block
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
It is possible to define a block with some sequential ports and some combinational ports.

In the example below, the ``single_port_ram_mixed`` has sequential input ports: ``we``, ``addr`` and ``data`` (which are controlled by ``clk``).

.. figure:: mixed_sp_ram.*
    :width: 75%

    Mixed sequential/combinational single port ram

However the output port (``out``) is a combinational output, connected internally to the ``we``, ``addr`` and ``data`` input registers.

.. code-block:: xml

      <model name="single_port_ram_mixed">
        <input_ports>
          <port name="we" clock="clk" combinational_sink_ports="out"/>
          <port name="addr" clock="clk" combinational_sink_ports="out"/>
          <port name="data" clock="clk" combinational_sink_ports="out"/>
          <port name="clk" is_clock="1"/>
        </input_ports>
        <output_ports>
          <port name="out"/>
        </output_ports>
      </model>


In the ``pb_type`` we define the external setup time of the input registers (50ps) as we did for :ref:`dff_timing_modeling`.
However, we also specify the following additional timing information:

 * The internal clock-to-q delay of the input registers (200ps)
 * The combinational delay from the input registers to the ``out`` port (800ps)

.. code-block:: xml

    <pb_type name="mem_sp" blif_model=".subckt single_port_ram_mixed" num_pb="1">
      <input name="addr" num_pins="9"/>
      <input name="data" num_pins="64"/>
      <input name="we" num_pins="1"/>
      <output name="out" num_pins="64"/>
      <clock name="clk" num_pins="1"/>

      <!-- External input register timing -->
      <T_setup value="50e-12" port="mem_sp.addr" clock="clk"/>
      <T_setup value="50e-12" port="mem_sp.data" clock="clk"/>
      <T_setup value="50e-12" port="mem_sp.we" clock="clk"/>

      <!-- Internal input register timing -->
      <T_clock_to_Q max="200e-12" port="mem_sp.addr" clock="clk"/>
      <T_clock_to_Q max="200e-12" port="mem_sp.data" clock="clk"/>
      <T_clock_to_Q max="200e-12" port="mem_sp.we" clock="clk"/>

      <!-- Internal combinational delay -->
      <delay_constant max="800e-12" in_port="mem_sp.addr" out_port="mem_sp.out"/>
      <delay_constant max="800e-12" in_port="mem_sp.data" out_port="mem_sp.out"/>
      <delay_constant max="800e-12" in_port="mem_sp.we" out_port="mem_sp.out"/>
    </pb_type>

.. _seq_sp_ram_timing_modeling:

Sequential block (with internal paths)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Some primitives represent more complex architecture primitives, which have timing paths contained completely within the block.

The model below specifies a sequential single-port RAM.
The ports ``we``, ``addr``, and ``data`` are sequential inputs, while the port ``out`` is a sequential output.
``clk`` is the common clock.

.. figure:: seq_sp_ram.*
    :width: 75%

    Sequential single port ram

.. code-block:: xml

      <model name="single_port_ram_seq">
        <input_ports>
          <port name="we" clock="clk" combinational_sink_ports="out"/>
          <port name="addr" clock="clk" combinational_sink_ports="out"/>
          <port name="data" clock="clk" combinational_sink_ports="out"/>
          <port name="clk" is_clock="1"/>
        </input_ports>
        <output_ports>
          <port name="out" clock="clk"/>
        </output_ports>
      </model>


Similarly to :ref:`mixed_sp_ram_timing_modeling` the ``pb_type`` defines the input register timing:

 * external input register setup time (50ps)
 * internal input register clock-to-q time (200ps)

Since the output port ``out`` is sequential we also define the:

 * internal *output* register setup time (60ps)
 * external *output* register clock-to-q time (300ps)

The combinational delay between the input and output registers is set to 740ps.

Note the internal path from the input to output registers can limit the maximum operating frequency.
In this case the internal path delay is 1ns (200ps + 740ps + 60ps) limiting the maximum frequency to 1 GHz.

.. code-block:: xml

    <pb_type name="mem_sp" blif_model=".subckt single_port_ram_seq" num_pb="1">
      <input name="addr" num_pins="9"/>
      <input name="data" num_pins="64"/>
      <input name="we" num_pins="1"/>
      <output name="out" num_pins="64"/>
      <clock name="clk" num_pins="1"/>

      <!-- External input register timing -->
      <T_setup value="50e-12" port="mem_sp.addr" clock="clk"/>
      <T_setup value="50e-12" port="mem_sp.data" clock="clk"/>
      <T_setup value="50e-12" port="mem_sp.we" clock="clk"/>

      <!-- Internal input register timing -->
      <T_clock_to_Q max="200e-12" port="mem_sp.addr" clock="clk"/>
      <T_clock_to_Q max="200e-12" port="mem_sp.data" clock="clk"/>
      <T_clock_to_Q max="200e-12" port="mem_sp.we" clock="clk"/>

      <!-- Internal combinational delay -->
      <delay_constant max="740e-12" in_port="mem_sp.addr" out_port="mem_sp.out"/>
      <delay_constant max="740e-12" in_port="mem_sp.data" out_port="mem_sp.out"/>
      <delay_constant max="740e-12" in_port="mem_sp.we" out_port="mem_sp.out"/>

      <!-- Internal output register timing -->
      <T_setup value="60e-12" port="mem_sp.out" clock="clk"/>

      <!-- External output register timing -->
      <T_clock_to_Q max="300e-12" port="mem_sp.out" clock="clk"/>
    </pb_type>

.. _seq_sp_ram_comb_inputs_timing_modeling:

Sequential block (with internal paths and combinational input)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
A primitive may have a mix of sequential and combinational inputs.

The model below specifies a mostly sequential single-port RAM.
The ports ``addr``, and ``data`` are sequential inputs, while the port ``we`` is a combinational input.
The port ``out`` is a sequential output.
``clk`` is the common clock.

.. figure:: seq_comb_sp_ram.*
    :width: 75%

    Sequential single port ram with a combinational input

.. code-block:: xml
    :emphasize-lines: 3

      <model name="single_port_ram_seq_comb">
        <input_ports>
          <port name="we" combinational_sink_ports="out"/>
          <port name="addr" clock="clk" combinational_sink_ports="out"/>
          <port name="data" clock="clk" combinational_sink_ports="out"/>
          <port name="clk" is_clock="1"/>
        </input_ports>
        <output_ports>
          <port name="out" clock="clk"/>
        </output_ports>
      </model>


We use register delays similar to :ref:`seq_sp_ram_timing_modeling`.
However we also specify the purely combinational delay between the combinational ``we`` input and sequential output ``out`` (800ps).
Note that the setup time of the output register still effects the ``we`` to ``out`` path for an effective delay of 860ps.

.. code-block:: xml
    :emphasize-lines: 17

    <pb_type name="mem_sp" blif_model=".subckt single_port_ram_seq_comb" num_pb="1">
      <input name="addr" num_pins="9"/>
      <input name="data" num_pins="64"/>
      <input name="we" num_pins="1"/>
      <output name="out" num_pins="64"/>
      <clock name="clk" num_pins="1"/>

      <!-- External input register timing -->
      <T_setup value="50e-12" port="mem_sp.addr" clock="clk"/>
      <T_setup value="50e-12" port="mem_sp.data" clock="clk"/>

      <!-- Internal input register timing -->
      <T_clock_to_Q max="200e-12" port="mem_sp.addr" clock="clk"/>
      <T_clock_to_Q max="200e-12" port="mem_sp.data" clock="clk"/>

      <!-- External combinational delay -->
      <delay_constant max="800e-12" in_port="mem_sp.we" out_port="mem_sp.out"/>

      <!-- Internal combinational delay -->
      <delay_constant max="740e-12" in_port="mem_sp.addr" out_port="mem_sp.out"/>
      <delay_constant max="740e-12" in_port="mem_sp.data" out_port="mem_sp.out"/>

      <!-- Internal output register timing -->
      <T_setup value="60e-12" port="mem_sp.out" clock="clk"/>

      <!-- External output register timing -->
      <T_clock_to_Q max="300e-12" port="mem_sp.out" clock="clk"/>
    </pb_type>


.. _multiclock_dp_ram_timing_modeling:

Multi-clock Sequential block (with internal paths)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
It is also possible for a sequential primitive to have multiple clocks.

The following model represents a multi-clock simple dual-port sequential RAM with:

 * one write port (``addr1`` and ``data1``, ``we1``) controlled by ``clk1``, and
 * one read port (``addr2`` and ``data2``) controlled by ``clk2``.

.. figure:: multiclock_dp_ram.*
    :width: 75%

    Multi-clock sequential simple dual port ram

.. code-block:: xml

      <model name="multiclock_dual_port_ram">
        <input_ports>
          <!-- Write Port -->
          <port name="we1" clock="clk1" combinational_sink_ports="data2"/>
          <port name="addr1" clock="clk1" combinational_sink_ports="data2"/>
          <port name="data1" clock="clk1" combinational_sink_ports="data2"/>
          <port name="clk1" is_clock="1"/>

          <!-- Read Port -->
          <port name="addr2" clock="clk2" combinational_sink_ports="data2"/>
          <port name="clk2" is_clock="1"/>
        </input_ports>
        <output_ports>
          <!-- Read Port -->
          <port name="data2" clock="clk2" combinational_sink_ports="data2"/>
        </output_ports>
      </model>

On the ``pb_type`` the input and output register timing is defined similarly to :ref:`seq_sp_ram_timing_modeling`, except multiple clocks are used.

.. code-block:: xml

    <pb_type name="mem_dp" blif_model=".subckt multiclock_dual_port_ram" num_pb="1">
      <input name="addr1" num_pins="9"/>
      <input name="data1" num_pins="64"/>
      <input name="we1" num_pins="1"/>
      <input name="addr2" num_pins="9"/>
      <output name="data2" num_pins="64"/>
      <clock name="clk1" num_pins="1"/>
      <clock name="clk2" num_pins="1"/>

      <!-- External input register timing -->
      <T_setup value="50e-12" port="mem_dp.addr1" clock="clk1"/>
      <T_setup value="50e-12" port="mem_dp.data1" clock="clk1"/>
      <T_setup value="50e-12" port="mem_dp.we1" clock="clk1"/>
      <T_setup value="50e-12" port="mem_dp.addr2" clock="clk2"/>

      <!-- Internal input register timing -->
      <T_clock_to_Q max="200e-12" port="mem_dp.addr1" clock="clk1"/>
      <T_clock_to_Q max="200e-12" port="mem_dp.data1" clock="clk1"/>
      <T_clock_to_Q max="200e-12" port="mem_dp.we1" clock="clk1"/>
      <T_clock_to_Q max="200e-12" port="mem_dp.addr2" clock="clk2"/>

      <!-- Internal combinational delay -->
      <delay_constant max="740e-12" in_port="mem_dp.addr1" out_port="mem_dp.data2"/>
      <delay_constant max="740e-12" in_port="mem_dp.data1" out_port="mem_dp.data2"/>
      <delay_constant max="740e-12" in_port="mem_dp.we1" out_port="mem_dp.data2"/>
      <delay_constant max="740e-12" in_port="mem_dp.addr2" out_port="mem_dp.data2"/>

      <!-- Internal output register timing -->
      <T_setup value="60e-12" port="mem_dp.data2" clock="clk2"/>

      <!-- External output register timing -->
      <T_clock_to_Q max="300e-12" port="mem_dp.data2" clock="clk2"/>
    </pb_type>

.. _clock_generator_timing_modeling:

Clock Generators
~~~~~~~~~~~~~~~~
Some blocks (such as PLLs) generate clocks on-chip.
To ensure that these generated clocks are identified as clock sources, the associated model output port should be marked with ``is_clock="1"``.


As an example consider the following simple PLL model:

.. code-block:: xml

    <model name="simple_pll">
      <input_ports>
        <port name="in_clock" is_clock="1"/>
      </input_ports>
      <output_ports>
        <port name="out_clock" is_clock="1"/>
      </output_ports>
    </model>

The port named ``in_clock`` is specified as a clock sink, since it is an input port with ``is_clock="1"``  set.

The port named ``out_clock`` is specified as a clock generator, since it is an *output* port with ``is_clock="1"`` set.

.. note:: Clock generators should not be the combinational sinks of primitive input ports.

Consider the following example netlist:

.. code-block:: none

    .subckt simple_pll \
        in_clock=clk \
        out_clock=clk_pll

Since we have specified that ``simple_pll.out_clock`` is a clock generator (see above), the user must specify what the clock relationship is between the input and output clocks.
This information must be either specified in the SDC file (if no SDC file is specified :ref:`VPR's default timing constraints <default_timing_constraints>` will be used instead).

.. note:: VPR has no way of determining what the relationship is between the clocks of a black-box primitive.

Consider the case where the ``simple_pll`` above creates an output clock which is 2 times the frequency of the input clock.
If the input clock period was 10ns then the SDC file would look like:

.. code-block:: tcl

    create_clock clk -period 10
    create_clock clk_pll -period 5                      #Twice the frequency of clk

It is also possible to specify in SDC that there is a phase shift between the two clocks:

.. code-block:: tcl

    create_clock clk -waveform {0 5} -period 10         #Equivalent to 'create_clock clk -period 10'
    create_clock clk_pll -waveform {0.2 2.7} -period 5  #Twice the frequency of clk with a 0.2ns phase shift


.. _clock_buffers_timing_modeling:

Clock Buffers & Muxes
~~~~~~~~~~~~~~~~~~~~~
Some architectures contain special primitives for buffering or controling clocks.
VTR supports modelling these using the ``is_clock`` attritube on the model to differentiate between 'data' and 'clock' signals, allowing users to control how clocks are traced through these primitives.

When VPR traces through the netlist it will propagate clocks from clock inputs to the downstream combinationally connected pins.

Clock Buffers/Gates
^^^^^^^^^^^^^^^^^^^
Consider the following black-box clock buffer with an enable:

.. code-block:: none

    .subckt clkbufce \
        in=clk3 \
        enable=clk3_enable \
        out=clk3_buf

We wish to have VPR understand that the ``in`` port of the ``clkbufce`` connects to the ``out`` port, and that as a result the nets ``clk3`` and ``clk3_buf`` are equivalent.

This is accomplished by tagging the ``in`` port as a clock (``is_clock="1"``), and combinationally connecting it to the ``out`` port (``combinational_sink_ports="out"``):

.. code-block:: xml

    <model name="clkbufce">
      <input_ports>
        <port name="in" combinational_sink_ports="out" is_clock="1"/>
        <port name="enable" combinational_sink_ports="out"/>
      </input_ports>
      <output_ports>
        <port name="out"/>
      </output_ports>
    </model>

With the corresponding pb_type:

.. code-block:: xml

    <pb_type name="clkbufce" blif_model="clkbufce" num_pb="1">
      <clock name="in" num_pins="1"/>
      <input name="enable" num_pins="1"/>
      <output name="out" num_pins="1"/>
      <delay_constant max="10e-12" in_port="clkbufce.in" out_port="clkbufce.out"/>
      <delay_constant max="5e-12" in_port="clkbufce.enable" out_port="clkbufce.out"/>
    </pb_type>

Notably, although the ``enable`` port is combinationally connected to the ``out`` port it will not be considered as a potential clock since it is not marked with ``is_clock="1"``.

Clock Muxes
^^^^^^^^^^^
Another common clock control block is a clock mux, which selects from one of several potential clocks.

For instance, consider:

.. code-block:: none

    .subckt clkmux \
        clk1=clka \
        clk2=clkb \
        sel=select \
        clk_out=clk_downstream

which selects one of two input clocks (``clk1`` and ``clk2``) to be passed through to (``clk_out``), controlled on the value of ``sel``.

This could be modelled as:

.. code-block:: xml

    <model name="clkmux">
      <input_ports>
        <port name="clk1" combinational_sink_ports="clk_out" is_clock="1"/>
        <port name="clk2" combinational_sink_ports="clk_out" is_clock="1"/>
        <port name="sel" combinational_sink_ports="clk_out"/>
      </input_ports>
      <output_ports>
        <port name="clk_out"/>
      </output_ports>
    </model>

    <pb_type name="clkmux" blif_model="clkmux" num_pb="1">
      <clock name="clk1" num_pins="1"/>
      <clock name="clk2" num_pins="1"/>
      <input name="sel" num_pins="1"/>
      <output name="clk_out" num_pins="1"/>
      <delay_constant max="10e-12" in_port="clkmux.clk1" out_port="clkmux.clk_out"/>
      <delay_constant max="10e-12" in_port="clkmux.clk2" out_port="clkmux.clk_out"/>
      <delay_constant max="20e-12" in_port="clkmux.sel" out_port="clkmux.clk_out"/>
    </pb_type>
  
where both input clock ports ``clk1`` and ``clk2`` are tagged with ``is_clock="1"`` and combinationally connected to the ``clk_out`` port.
As a result both nets ``clka`` and ``clkb`` in the netlist would be identified as independent clocks feeding ``clk_downstream``.

.. note::

    Clock propagation is driven by netlist connectivity so if one of the input clock ports (e.g. ``clk1``) was disconnected in the netlist no associated clock would be created/considered.

Clock Mux Timing Constraints
""""""""""""""""""""""""""""

For the clock mux example above, if the user specified the following :ref:`SDC timing constraints <sdc_commands>`:

.. code-block:: tcl
    
    create_clock -period 3 clka
    create_clock -period 2 clkb

VPR would propagate both ``clka`` and ``clkb`` through the clock mux.
Therefore the logic connected to ``clk_downstream`` would be analyzed for both the ``clka`` and ``clkb`` constraints.

Most likely (unless ``clka`` and ``clkb`` are used elswhere) the user should additionally specify:

.. code-block:: tcl
   
    set_clock_groups -exclusive -group clka -group clkb

Which avoids analyzing paths between the two clocks (i.e. ``clka`` -> ``clkb`` and ``clkb`` -> ``clka``) which are not physically realizable.
The muxing logic means only one clock can drive ``clk_downstream`` at any point in time (i.e. the mux enforces that ``clka`` and ``clkb`` are mutually exclusive).
This is the behaviour of :ref:`VPR's default timing constraints <default_timing_constraints>`.
