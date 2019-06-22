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
To ensure that these generated clocks are identified as clocks, the associated model output port should be marked with ``is_clock="1"``.

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
