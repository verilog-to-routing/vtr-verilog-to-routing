.. _parmys_plugin:

Parmys Plugin
===============

Parmys (Partial Mapper for Yosys) is a Yosys plugin that performs intelligent partial mapping (inference, binding, and hard/soft logic trade-offs) from Odin-II.
Please see `Parmys-Plugin GitHub <https://github.com/CAS-Atlantic/parmys-plugin.git>`_ repository for more information.

Available parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. option:: -a ARCHITECTURE_FILE

    VTR FPGA architecture description file (XML)

.. option:: -c XML_CONFIGURATION_FILE

    Configuration file

.. option:: -top top_module

    set the specified module as design top module

.. option:: -nopass

    No additional passes will be executed.

.. option:: -exact_mults int_value

    To enable mixing hard block and soft logic implementation of adders

.. option:: -mults_ratio float_value

    To enable mixing hard block and soft logic implementation of adders

.. option:: -vtr_prim

    No additional passes will be executed.

.. option:: -vtr_prim

    loads vtr primitives as modules, if the design uses vtr primitives then this flag is mandatory for first run

