.. _fpga_architecture_description:

FPGA Architecture Description
=============================

VTR uses an XML-based architecture description language to describe the targeted FPGA architecture.
This flexible description language allows the user to describe a large number of hypothetical and commercial-like FPGA architectures.

An interactive visualizer is available for exploring VTR FPGA architecture description files.
It provides a device-wide, inter-tile grid view and a detailed intra-tile view of primitives and local interconnect,
as well as many other views,
making it easier to develop and debug architecture XML files without running the full VPR flow.

* **Web application (no install required):** `Open in Browser <https://fpga-architecture-visualizer.web.app/>`_
* **Source code and native binaries (Windows, Linux, macOS):** `GitHub Repository <https://github.com/AlexandreSinger/rust-fpga-arch-visualizer>`_

See the :ref:`arch_tutorial` for an introduction to the architecture description language.
For a detailed reference on the supported options see the :ref:`arch_reference`.

.. toctree::
   :maxdepth: 2

   reference
   example_arch
