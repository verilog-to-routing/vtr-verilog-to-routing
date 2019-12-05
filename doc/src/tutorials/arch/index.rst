.. _arch_tutorial:

Architecture Modeling
---------------------

This page provides information on the FPGA architecture description language used by VPR.
This page is geared towards both new and experienced users of vpr.

New users may wish to consult the conference paper that introduces the language :cite:`luu_architecture_description_lanage`.
This paper describes the motivation behind this new language as well as a short tutorial on how to use the language to describe different complex blocks of an FPGA.

New and experienced users alike should consult the detailed :ref:`arch_reference` which serves to documents every property of the language.

Multiple examples of how this language can be used to describe different types of complex blocks are provided as follows:

**Complete Architecture Description Walkthrough Examples:**

.. toctree::
   :maxdepth: 1

   classic_soft_logic
   configurable_memory_bus
   fracturable_multiplier_bus

**Architecture Description Examples:**

.. toctree::
   :maxdepth: 1

   fracturable_multiplier
   configurable_memory
   xilinx_virtex_6_like

**Modeling Guides:**

.. toctree::
    :maxdepth: 1

    timing_modeling/index
