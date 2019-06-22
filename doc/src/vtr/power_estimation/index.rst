.. _power_estimation:

Power Estimation
================

VTR provides transistor-level dynamic and static power estimates for a given architecture and circuit.

:numref:`fig_power_estimation_flow` illustrates how power estimation is performed in the VTR flow.
The actual power estimation is performed within the :ref:`VPR` executable; however, additional files must be provided.
In addition to the circuit and architecture files, power estimation requires files detailing the signal activities and technology properties.

:ref:`vtr_running_power` details how to run power estimation for VTR.
:ref:`power_support_tools` provides details on the supporting tools that are used to generate the signal activities and technology properties files.
:ref:`power_arch_modeling` provides details about how the tool models architectures, including different modelling methods and options.
:ref:`power_advanced_usage` provides more advanced configuration options.


.. _fig_power_estimation_flow:

.. figure:: power_flow.*

    Power Estimation in the VTR Flow

.. _vtr_running_power:

Running VTR with Power Estimation
---------------------------------

VTR Flow
~~~~~~~~

The easiest way to run the VTR flow is to use the :ref:`run_vtr_flow` script.

In order to perform power estimation, you must add the following options:

  * :option:`run_vtr_flow.pl -power`
  * :option:`run_vtr_flow.pl -cmos_tech` ``<cmos_tech_properties_file>``

The CMOS technology properties file is an XML file that contains relevant process-dependent information needed for power estimation.
XML files for 22nm, 45nm, and 130nm PTM models can be found here::

$VTR_ROOT/vtrflow/tech/*

See :ref:`power_technology_properties` for information on how to generate an XML file for your own SPICE technology model.

VPR
~~~

Power estimation can also be run directly from VPR with the following (all required) options:

* :option:`vpr --power`: Enables power estimation.
* :option:`vpr --activity_file` ``<activities.act>``: The activity file, produce by ACE 2.0, or another tool.
* :option:`vpr --tech_properties` ``<tech_properties.xml>``: The technology properties file.

Power estimation requires an activity file, which can be generated as described in :ref:`power_ace`.

.. _power_support_tools:

Supporting Tools
----------------

.. _power_technology_properties:

Technology Properties
~~~~~~~~~~~~~~~~~~~~~

Power estimation requires information detailing the properties of the CMOS technology.
This information, which includes transistor capacitances, leakage currents, etc. is included in an ``.xml`` file, and provided as a parameter to VPR.
This XML file is generated using a script which automatically runs HSPICE, performs multiple circuit simulations, and extract the necessary values.

Some of these technology XML files are included with the release, and are located here::

    $VTR_ROOT/vtr_flow/tech/*

If the user wishes to use a different CMOS technology file, they must run the following script:

.. note:: HSPICE must be available on the users path

.. code-block:: none

    $VTR_ROOT/vtr_flow/scripts/generate_cmos_tech_data.pl <tech_file> <tech_size> <vdd> <temp>


where:

    * ``<tech_file>``: Is a SPICE technology file, containing a ``pmos`` and ``nmos`` models.

    * ``<tech_size>``: The technology size, in meters.

        **Example:**

        A 90nm technology would have the value ``90e-9``.

    * ``<vdd>``: Supply voltage in Volts.

    * ``<temp>``: Operating temperature, in Celcius.


.. _power_ace:

ACE 2.0 Activity Estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Power estimation requires activity information for the entire netlist.
This ativity information consists of two values:

#. *The Signal Probability*, :math:`P_1`, is the long-term probability that a signal is logic-high.

   **Example:**

   A clock signal with a 50% duty cycle will have :math:`P_1(clk) = 0.5`.

#. *The Transition Density* (or switching activity), :math:`A_S`, is the average number of times the signal will switch during each clock cycle.

   **Example:**

   A clock has :math:`A_S(clk)=2`.

The default tool used to perform activity estimation in VTR is ACE 2.0 :cite:`lamoureux_activity_estimation`.
This tool was originally designed to work with the (now obsolete) Berkeley SIS tool
ACE 2.0 was modifed to use ABC, and is included in the VTR package here::

    $VTR_ROOT/ace2

The tool can be run using the following command-line arguments::

    $VTR_ROOT/ace2/ace -b <abc.blif> -c <clock_name> -o <activities.act> -n <new.blif>

where

    * ``<abc.blif>``: Is the input BLIF file produced by ABC.
    * ``<clock_name>``: Is the name of the clock in the input BLIF file
    * ``<activities.act>``: Is the activity file to be created.
    * ``<new.blif>``: The new BLIF file.

        This will be functionally identical in function to the ABC blif; however, since ABC does not maintain internal node names, a new BLIF must be produced with node names that match the activity file.

User’s may with to use their own activity estimation tool.
The produced activity file must contain one line for each net in the BLIF file, in the following format::

    <net name> <signal probability> <transistion density>

.. _power_arch_modeling:

Architecture Modelling
----------------------

The following section describes the architectural assumptions made by the power model, and the related parameters in the architecture file.

Complex Blocks
~~~~~~~~~~~~~~

The VTR architecture description language supports a hierarchichal description of blocks. In the architecture file, each block is described as a ``pb_type``, which may includes one or more children of type ``pb_type``, and interconnect structures to connect them.

The power estimation algorithm traverses this hierarchy recursively, and performs power estimation for each ``pb_type``.
The power model supports multiple power estimation methods, and the user specifies the desired method in the architecture file:

.. code-block:: xml

    <pb_type>
        <power method="<estimation-method>"/>
    </pb_type>

The following is a list of valid estimation methods.
Detailed descriptions of each type are provided in the following sections.
The methods are listed in order from most accurate to least accurate.

#. ``specify-size``: Detailed transistor level modelleling.

   The user supplies all buffer sizes and wire-lengths.
   Any not provided by the user are ignored.

#. ``auto-size``: Detailed transistor level modelleling.

   The user can supply buffer sizes and wire-lengths; however, they will be automatically inserted when not provided.

#. ``pin-toggle``: Higher-level modelling.

   The user specifies energy per toggle of the pins.
   Static power provided as an absolute.

#. ``C-internal``: Higher-level modelling.

   The user supplies the internal capacitance of the block.
   Static power provided as an absolute.

#. ``absolute``: Highest-level modelling.

   The user supplies both dynamic and static power as absolutes.

Other methods of estimation:

#. ``ignore``: The power of the ``pb_type`` is ignored, including any children.

#. ``sum-of-children``: Power of ``pb_type`` is solely the sum of all children ``pb_types``.

    Interconnect between the ``pb_type`` and its children is ignored.

.. note:: If no estimation method is provided, it is inherited from the parent ``pb_type``.

.. note:: If the top-level ``pb_type`` has no estimation method, ``auto-size`` is assumed.


``specify-size``
~~~~~~~~~~~~~~~~
This estimation method provides a detailed transistor level modelling of CLBs, and will provide the most accurate power estimations.
For each ``pb_type``, power estimation accounts for the following components (see :numref:`fig_power_sample_block`).

    * Interconnect multiplexers

    * Buffers and wire capacitances

    * Child ``pb_types``


.. _fig_power_sample_block:

.. figure:: power_sample_clb.*

    Sample Block

**Multiplexers:**
Interconnect multiplexers are modelled as 2-level pass-transistor multiplexers, comprised of minimum-size NMOS transistors.
Their size is determined automatically from the ``<interconnect/>`` structures in the architecture description file.

**Buffers and Wires:**
Buffers and wire capacitances are not defined in the architecture file, and must be explicitly added by the user.
They are assigned on a per port basis using the following construct:

.. code-block:: xml

    <pb_type>
        <input name="my_input" num_pins="1">
            <power ...options.../>
        </input>
    </pb_type>

The wire and buffer attributes can be set using the following options.
If no options are set, it is assumed that the wire capacitance is zero, and there are no buffers present.
Keep in mind that the port construct allows for multiple pins per port.
These attributes will be applied to each pin in the port.
If necessary, the user can seperate a port into multiple ports with different wire/buffer properties.

* ``wire_capacitance=1.0e-15``: The absolute capacitance of the wire, in Farads.

* ``wire_length=1.0e-7``: The absolute length of the wire, in meters.

    The local interconnect capacitance option must be specified, as described in :ref:`power_local_interconnect_capacitance`.

* ``wire_length=auto``: The wirelength is automatically sized. See :ref:`power_local_wire_auto_sizing`.

* ``buffer_size=2.0``: The size of the buffer at this pin. See for more :ref:`power_buffer_sizing` information.

* ``buffer_size=auto``: The size of the buffer is automatically sized, assuming it drives the above wire capacitance and a single multiplexer. See :ref:`power_buffer_sizing` for more information.

**Primitives:**
For all child ``pb_types``, the algorithm performs a recursive call.
Eventually ``pb_types`` will be reached that have no children.
These are primitives, such as flip-flops, LUTs, or other hard-blocks.
The power model includes functions to perform transistor-level power estimation for flip-flops and LUTs.
If the user wishes to use a design with other primitive types (memories, multipliers, etc), they must provide an equivalent function.
If the user makes such a function, the ``power_calc_primitive`` function should be modified to call it.
Alternatively, these blocks can be configured to use higher-level power estimation methods.

``auto-size``
~~~~~~~~~~~~~
This estimation method also performs detailed transistor-level modelling.
It is almost identical to the ``specify-size`` method described above.
The only difference is that the local wire capacitance and buffers are automatically inserted for all pins, when necessary.
This is equivalent to using the ``specify-size`` method with the ``wire_length=auto`` and ``buffer_size=auto`` options for every port.

.. note:: **This is the default power estimation method.**

Although not as accurate as user-provided buffer and wire sizes, it is capable of automatically capturing trends in power dissipation as architectures are modified.

``pin-toggle``
~~~~~~~~~~~~~~
This method allows users to specify the dynamic power of a block in terms of the energy per toggle (in Joules) of each input, output or clock pin for the ``pb_type``.
The static power is provided as an absolute (in Watts).
This is done using the following construct:

.. code-block:: xml

    <pb_type>
        ...
        <power method="pin-toggle">
            <port name="A" energy_per_toggle="1.0e-12"/>
            <port name="B[3:2]" energy_per_toggle="1.0e-12"/>
            <port name="C" energy_per_toggle="1.0e-12" scaled_by_static_porb="en1"/>
            <port name="D" energy_per_toggle="1.0e-12" scaled_by_static_porb_n="en2"/>
            <static_power power_per_instance="1.0e-6"/>
        </power>
    </pb_type>

Keep in mind that the port construct allows for multiple pins per port.
Unless an subset index is provided, the energy per toggle will be applied to each pin in the port.
The energy per toggle can be scaled by another signal using the ``scaled_by_static_prob``.
For example, you could scale the energy of a memory block by the read enable pin.
If the read enable were high 80% of the time, then the energy would be scaled by the :math:`signal\_probability`, 0.8.
Alternatively ``scaled_by_static_prob_n`` can be used for active low signals, and the energy will be scaled by :math:`(1-signal\_probability)`.

This method does not perform any transistor-level estimations; the entire power estimation is performed using the above values.
It is assumed that the power usage specified here includes power of all child ``pb_types``.
No further recursive power estimation will be performed.

``C-internal``
~~~~~~~~~~~~~~
This method allows the users to specify the dynamic power of a block in terms of the internal capacitance of the block.
The activity will be averaged across all of the input pins, and will be supplied with the internal capacitance to the standard equation:

.. math::
    P_{dyn}=\frac{1}{2}\alpha CV^2.

Again, the static power is provided as an absolute (in Watts).
This is done using the following construct:

.. code-block:: xml

    <pb_type>
        <power method="c-internal">
            <dynamic_power C_internal="1.0e-16"/>
            <static_power power_per_instance="1.0e-16"/>
        </power>
    </pb_type>

It is assumed that the power usage specified here includes power of all child ``pb_types``.
No further recursive power estimation will be performed.

``absolute``
~~~~~~~~~~~~
This method is the most basic power estimation method, and allows users to specify both the dynamic and static power of a block as absolute
values (in Watts).
This is done using the following construct:

.. code-block:: xml

    <pb_type>
        <power method="absolute">
            <dynamic_power power_per_instance="1.0e-16"/>
            <static_power power_per_instance="1.0e-16"/>
        </power>
    </pb_type>

It is assumed that the power usage specified here includes power of all child ``pb_types``.
No further recursive power estimation will be performed.

Global Routing
--------------

Global routing consists of switch boxes and input connection boxes.

Switch Boxes
~~~~~~~~~~~~

Switch boxes are modelled as the following components (:numref:`fig_power_sb`):

#. Multiplexer
#. Buffer
#. Wire capacitance

.. _fig_power_sb:

.. figure:: power_sb.*

    Switch Box

**Multiplexer:**
The multiplexer is modelled as 2-level pass-transistor multiplexer, comprised of minimum-size NMOS transistors.
The number of inputs to the multiplexer is automatically determined.

**Buffer:**
The buffer is a multistage CMOS buffer.
The buffer size is determined based upon output capacitance provided in the architecture file:

.. code-block:: xml

    <switchlist>
        <switch type="mux" ... C_out="1.0e-16"/>
    </switchlist>

The user may override this method by providing the buffer size as shown below:

.. code-block:: xml

    <switchlist>
        <switch type="mux" ... power_buf_size="16"/>
    </switchlist>

The size is the drive strength of the buffer, relative to a minimum-sized inverter.

Input Connection Boxes
~~~~~~~~~~~~~~~~~~~~~~

Input connection boxes are modelled as the following components (:numref:`fig_power_cb`):

* One buffer per routing track, sized to drive the load of all input multiplexers to which the buffer is connected (For buffer sizing see :ref:`power_buffer_sizing`).

* One multiplexer per block input pin, sized according to the number of routing tracks that connect to the pin.

.. _fig_power_cb:

.. figure:: power_cb.*

    Connection Box

Clock Network
~~~~~~~~~~~~~

The clock network modelled is a four quadrant spine and rib design, as illustrated in :numref:`fig_power_clock_network`.
At this time, the power model only supports a single clock.
The model assumes that the entire spine and rib clock network will contain buffers separated in distance by the length of a grid tile.
The buffer sizes and wire capacitances are specified in the architecture file using the following construct:

.. code-block:: xml

    <clocks>
        <clock ... clock_options ... />
    </clocks>

The following clock options are supported:

* ``C_wire=1e-16``: The absolute capacitance, in fards, of the wire between each clock buffer.

* ``C_wire_per_m=1e-12``: The wire capacitance, in fards per m.

    The capacitance is calculated using an automatically determined wirelength, based on the area of a tile in the FPGA.

* ``buffer_size=2.0``: The size of each clock buffer.

    This can be replaced with the ``auto`` keyword.
    See :ref:`power_buffer_sizing` for more information on buffer sizing.



.. _fig_power_clock_network:

.. figure:: power_clock_network.*

    The clock network. Squares represent CLBs, and the wires represent the clock network.


.. _power_advanced_usage:

Other Architecture Options & Techniques
---------------------------------------

.. _power_local_wire_auto_sizing:

Local Wire Auto-Sizing
~~~~~~~~~~~~~~~~~~~~~~

Due to the significant user effort required to provide local buffer and wire sizes, we developed an algorithm to estimate them automatically.
This algorithm recursively calculates the area of all entities within a CLB, which consists of the area of primitives and the area of local interconnect multiplexers.
If an architecture uses new primitives in CLBs, it should include a function that returns the transistor count.
This function should be called from within ``power_count_transistors_primitive()``.

In order to determine the wire length that connects a parent entity to its children, the following assumptions are made:

*  Assumption 1:
    All components (CLB entities, multiplexers, crossbars) are assumed to be contained in a square-shaped area.

*  Assumption 2:
    All wires connecting a parent entity to its child pass through the *interconnect square*, which is the sum area of all interconnect multiplexers belonging to the parent entity.

:numref:`fig_power_local_interconnect` provides an illustration of a parent entity connected to its child entities, containing one of each interconnect type (direct, many-to-1, and complete).
In this figure, the square on the left represents the area used by the transistors of the interconnect multiplexers.
It is assumed that all connections from parent to child will pass through this area.
Real wire lengths could me more or less than this estimate; some pins in the parent may be directly adjacent to child entities, or they may have to traverse a distance greater than just the interconnect area.
Unfortuantely, a more rigorous estimation would require some information about the transistor layout.

.. _fig_power_local_interconnect:

.. figure:: power_local_interconnect.*

    Local interconnect wirelength.



.. _table_power_inerconnect_wire_cap:

.. table:: Local interconnect wirelength and capacitance. :math:`C_{inv}` is the input capacitance of a minimum-sized inverter.

    ==============================  ===========================================  =======================
    Connection from Entity Pin to:  Estimated Wirelength                         Transistor Capacitance
    ==============================  ===========================================  =======================
    Direct (Input or Output)        :math:`0.5 \cdot L_{interc}`                 0
    Many-to-1 (Input or Output)     :math:`0.5 \cdot L_{interc}`                 :math:`C_{INV}`
    Complete *m:n* (Input)          :math:`0.5 \cdot L_{interc} + L_{crossbar}`  :math:`n \cdot C_{INV}`
    Complete *m:n* (Output)         :math:`0.5 \cdot L_{interc}`                 :math:`C_{INV}`
    ==============================  ===========================================  =======================

:numref:`table_power_inerconnect_wire_cap` details how local wire lengths are determined as a function of entity and interconnect areas.
It is assumed that each wire connecting a pin of a ``pb_type`` to an interconnect structure is of length :math:`0.5 \cdot L_{interc}`.
In reality, this length depends on the actual transistor layout, and may be much larger or much smaller than the estimated value.
If desired, the user can override the 0.5 constant in the architecture file:

.. code-block:: xml

    <architecture>
        <power>
            <local_interconnect factor="0.5"/>
        </power>
    </architecture>


.. _power_buffer_sizing:

Buffer Sizing
~~~~~~~~~~~~~

In the power estimator, a buffer size refers to the size of the final stage of multi-stage buffer (if small, only a single stage is used).
The specified size is the :math:`\frac{W}{L}` of the NMOS transistor.
The PMOS transistor will automatically be sized larger.
Generally, buffers are sized depending on the load capacitance, using the following equation:

.. math::

       \text{Buffer Size} = \frac{1}{2 \cdot f_{LE}} * \frac{C_{Load}}{C_{INV}}

In this equation, :math:`C_{INV}` is the input capacitance of a minimum-sized inverter, and :math:`f_{LE}` is the logical effort factor.
The logical effort factor is the gain between stages of the multi-stage buffer, which by default is 4 (minimal delay).
The term :math:`(2\cdot f_{LE})` is used so that the ratio of the final stage to the driven capacitance is smaller.
This produces a much lower-area, lower-power buffer that is still close to the optimal delay, more representative of common design practises.
The logical effort factor can be modified in the architecture file:

.. code-block:: xml

    <architecture>
        <power>
            <buffers logical_effor_factor="4"/>
        </power>
    </architecture>

.. _power_local_interconnect_capacitance:

Local Interconnect Capacitance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If using the ``auto-size`` or ``wire-length`` options (:ref:`power_arch_modeling`), the local interconnect capacitance must be specified.
This is specified in the units of Farads/meter.

.. code-block:: xml

    <architecture>
        <power>
            <local_interconnect C_wire="2.5e-15"/>
        </power>
    </architecture>

