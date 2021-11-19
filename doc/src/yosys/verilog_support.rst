.. _verilog_support:

Verilog Support
===============

Yosys RTL framework has extensive Verilog IEEE-2005 support. Please see the `Yosys GitHub <https://github.com/YosysHQ/yosys#unsupported-verilog-2005-features>`_ repository for more information on a few unsupported Verilog-2005 features.

Unsupported Verilog-2005 features by Yosys
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

	└── Yosys
		├── Unsupported Features
		│	└── Non-synthesizable language features as defined in IEC 62142(E):2005 / IEEE Std. 1364.1(E):2002
		│
		├── Net Types
		│	├── tri
		│	├── triand
		│	└── trior
		│
		├── Keywords
		│	├── config
		│	└── disable
		│		
		└── Library files