.. _get_vtr:

Get VTR
-----------

How to Cite
~~~~~~~~~~~

Citations are important in academia, as they ensure contributors receive credit for their efforts.
Therefore please use the following paper as a general citation whenever you use VTR:
    
    M. A. Elgammal, A. Mohaghegh, S. G. Shahrouz, F. Mahmoudi, F. Koşar, K. Talaei, J. Fife, D. Khadivi, K. E. Murray, A. Boutros, K.B. Kent, J. Goeders, V. Betz "VTR 9: Open-Source CAD for Fabric and Beyond FPGA Architecture Exploration" ACM TRETS, Vol. 13, No. 3, Sept. 2025, pp. 1 - 53.

Bibtex:

.. code-block:: none

    @article{vtr9,
      title={VTR 9: Open-Source CAD for Fabric and Beyond FPGA Architecture Exploration},
      author={Elgammal, Mohamed A. and Mohaghegh, Amin and Shahrouz, Soheil G. and Mahmoudi, Fatemehsadat and Kosar, Fahrican and Talaei, Kimia and Fife, Joshua and Khadivi, Daniel and Murray, Kevin and Boutros, Andrew and Kent, Kenneth B. and Goeders, Jeff and Betz, Vaughn},
      journal={ACM Trans. Reconfigurable Technol. Syst.},
      issue_date = {September 2025},
      volume = {18},
      number = {3},
      numpages = {53},
      year={2025}
    }

We are always interested in how VTR is being used, so feel free email the `vtr-users <https://verilogtorouting.org/contact/>`_ list with how you are using VTR.

Download
~~~~~~~~

The official VTR release is available from:

    https://verilogtorouting.org/download

VTR Docker Image
~~~~~~~~~~~~~~~~
A docker image for VTR is available. This image provides all the required packages and python libraries required. However, this ease to compile and run comes at the cost of some runtime increase (<10%). To pull and run the docker image of latest VTR repository, you can run the following commands:

.. code-block:: bash

    > sudo docker pull mohamedelgammal/vtr-master:latest
    > sudo docker run -it mohamedelgammal/vtr-master:latest


Release
~~~~~~~

The VTR |version| release provides the following:

* benchmark circuits,
* sample FPGA architecture description files,
* the full CAD flow, and
* scripts to run that flow.

The FPGA CAD flow takes as input, a user circuit (coded in Verilog) and a description of the FPGA architecture.
The CAD flow then maps the circuit to the FPGA architecture to produce, as output, a placed-and-routed FPGA.
Here are some highlights of the |version| full release:

* Timing-driven logic synthesis, packing, placement, and routing with multi-clock support.

* Power Analysis

* Benchmark digital circuits consisting of real applications that contain both memories and multipliers.

  Seven of the 19 circuits contain more than 10,000 6-LUTs. The largest of which is just under 100,000 6-LUTs.

* Sample architecture files of a wide range of different FPGA architectures including:

    #. Timing annotated architectures
    #. Various fracturable LUTs (dual-output LUTs that can function as one large LUT or two smaller LUTs with some shared inputs)
    #. Various configurable embedded memories and multiplier hard blocks
    #. One architecture containing embedded floating-point cores, and
    #. One architecture with carry chains.

* A front-end Verilog elaborator that has support for hard blocks.

  This tool can automatically recognize when a memory or multiplier instantiated in a user circuit is too large for a target FPGA architecture.
  When this happens, the tool can automatically split that memory/multiplier into multiple smaller components (with some glue logic to tie the components together).
  This makes it easier to investigate different hard block architectures because one does not need to modify the Verilog if the circuit instantiates a memory/multiplier that is too large.

* Packing/Clustering support for FPGA logic blocks with widely varying functionality.

  This includes memories with configurable aspect ratios, multipliers blocks that can fracture into smaller multipliers, soft logic clusters that contain fracturable LUTs, custom interconnect within a logic block, and more.

* Ready-to-run scripts that guide a user through the complexities of building the tools as well as using the tools to map realistic circuits (written in Verilog) to FPGA architectures.

* Regression tests of experiments that we have conducted to help users error check and/or compare their work.

  Along with experiments for more conventional FPGAs, we also include an experiment that explores FPGAs with embedded floating-point cores investigated in :cite:`ho_floating_point_fpga` to illustrate the usage of the VTR framework to explore unconventional FPGA architectures.

Development Repository
~~~~~~~~~~~~~~~~~~~~~~
The development repository for the Verilog-to-Routing project is hosted at:

    https://github.com/verilog-to-routing/vtr-verilog-to-routing

Unlike the nicely packaged official releases the code in a constant state of flux.
You should expect that the tools are not always stable and that more work is needed to get the flow to run.
