.. _titanium_benchmarks_tutorial:

Running the Titanium Benchmarks
--------------------------------

This tutorial describes how to run the :ref:`Titanium benchmarks <titanium_benchmarks>` with VTR.

Integrating the Titanium Benchmarks into VTR
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Titanium benchmarks are distributed as part of the Titan benchmark release and are not included directly with VTR due to their size.

They can be automatically integrated into the VTR source tree by running the following from the root of the VTR source tree:

.. code-block:: console

    $ make get_titan_benchmarks

Once completed, the Titanium benchmark BLIF netlists can be found under ``$VTR_ROOT/vtr_flow/benchmarks/titan_blif/titanium/``, and the Stratix 10 architecture under ``$VTR_ROOT/vtr_flow/arch/titan/stratix10_arch.timing.xml``.

.. note:: :term:`$VTR_ROOT` corresponds to the root of the VTR source tree.

Running Benchmarks Manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the benchmarks have been integrated into VTR they can be run manually.

Each benchmark is constrained to the smallest Stratix 10 device that can fit it, and all benchmarks use a fixed channel width of 400 tracks.

For example, the following uses :ref:`VPR` to implement the ``sobel`` benchmark onto the ``stratix10_arch.timing.xml`` architecture, constrained to the ``1SX065HH1F35E1VG`` device at a :option:`channel width <vpr --route_chan_width>` of 400 tracks:

.. code-block:: console

    $ vpr $VTR_ROOT/vtr_flow/arch/titan/stratix10_arch.timing.xml \
          $VTR_ROOT/vtr_flow/benchmarks/titan_blif/titanium/stratix10/sobel_stratix10_arch_timing.blif \
          --device 1SX065HH1F35E1VG \
          --route_chan_width 400 \
          --max_router_iterations 400

For the Titanium benchmarks, we usually increase the max router iterations above the default to 400 to increase the effort of the router for these larger benchmarks.

Running All Benchmarks with VTR Flow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All Titanium benchmarks can be run using the VTR flow infrastructure with the following task configuration:

.. code-block:: ini

    # Path to directory of circuits to use
    circuits_dir=benchmarks/titan_blif/titanium/stratix10

    # Path to directory of SDCs to use
    sdc_dir=benchmarks/titan_blif/titanium/stratix10

    # Path to directory of architectures to use
    archs_dir=arch/titan

    # Add circuits to list to sweep
    # circuit_list_add=mem_tester_max_stratix10_arch_timing.blif
    circuit_list_add=rocket31_stratix10_arch_timing.blif
    circuit_list_add=ASU_LRN_stratix10_arch_timing.blif
    circuit_list_add=ChainNN_LRN_LG_stratix10_arch_timing.blif
    circuit_list_add=ChainNN_ELT_LG_stratix10_arch_timing.blif
    circuit_list_add=ChainNN_BSC_LG_stratix10_arch_timing.blif
    circuit_list_add=rocket17_stratix10_arch_timing.blif
    circuit_list_add=ASU_ELT_stratix10_arch_timing.blif
    circuit_list_add=ASU_BSC_stratix10_arch_timing.blif
    circuit_list_add=tdfir_stratix10_arch_timing.blif
    circuit_list_add=pricing_stratix10_arch_timing.blif
    circuit_list_add=mem_tester_stratix10_arch_timing.blif
    circuit_list_add=mandelbrot_stratix10_arch_timing.blif
    circuit_list_add=channelizer_stratix10_arch_timing.blif
    circuit_list_add=fft1d_offchip_stratix10_arch_timing.blif
    circuit_list_add=DLA_LRN_stratix10_arch_timing.blif
    circuit_list_add=matrix_mult_stratix10_arch_timing.blif
    circuit_list_add=fft1d_stratix10_arch_timing.blif
    circuit_list_add=fft2d_stratix10_arch_timing.blif
    circuit_list_add=neko_stratix10_arch_timing.blif
    circuit_list_add=DLA_ELT_stratix10_arch_timing.blif
    circuit_list_add=DLA_BSC_stratix10_arch_timing.blif
    circuit_list_add=jpeg_deco_stratix10_arch_timing.blif
    circuit_list_add=nyuzi_stratix10_arch_timing.blif
    circuit_list_add=sobel_stratix10_arch_timing.blif

    # Constrain the circuits to their devices
    circuit_constraint_list_add=(rocket31_stratix10_arch_timing.blif,       device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(ASU_LRN_stratix10_arch_timing.blif,        device=1SG211HN1F43E1VG)
    circuit_constraint_list_add=(ChainNN_LRN_LG_stratix10_arch_timing.blif, device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(ChainNN_ELT_LG_stratix10_arch_timing.blif, device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(ChainNN_BSC_LG_stratix10_arch_timing.blif, device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(rocket17_stratix10_arch_timing.blif,       device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(ASU_ELT_stratix10_arch_timing.blif,        device=1SG211HN1F43E1VG)
    circuit_constraint_list_add=(ASU_BSC_stratix10_arch_timing.blif,        device=1SG211HN1F43E1VG)
    circuit_constraint_list_add=(tdfir_stratix10_arch_timing.blif,          device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(pricing_stratix10_arch_timing.blif,        device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(mem_tester_stratix10_arch_timing.blif,     device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(mandelbrot_stratix10_arch_timing.blif,     device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(channelizer_stratix10_arch_timing.blif,    device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(fft1d_offchip_stratix10_arch_timing.blif,  device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(DLA_LRN_stratix10_arch_timing.blif,        device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(matrix_mult_stratix10_arch_timing.blif,    device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(fft1d_stratix10_arch_timing.blif,          device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(fft2d_stratix10_arch_timing.blif,          device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(neko_stratix10_arch_timing.blif,           device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(DLA_ELT_stratix10_arch_timing.blif,        device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(DLA_BSC_stratix10_arch_timing.blif,        device=1SX110HN1F43E1VG)
    circuit_constraint_list_add=(jpeg_deco_stratix10_arch_timing.blif,      device=1SG280HH1F55E1VG)
    circuit_constraint_list_add=(nyuzi_stratix10_arch_timing.blif,          device=1SX040HH1F35E1VG)
    circuit_constraint_list_add=(sobel_stratix10_arch_timing.blif,          device=1SX065HH1F35E1VG)

    # Add architectures to list to sweep
    arch_list_add=stratix10_arch.timing.xml

    # Parse info and how to parse
    parse_file=vpr_titan_s10.txt

    # How to parse QoR info
    qor_parse_file=qor_ap_fixed_chan_width.txt

    # Pass requirements
    pass_requirements_file=pass_requirements.txt

    # Pass the script params
    script_params=-starting_stage vpr --route_chan_width 400 --max_router_iterations 400

.. note::
    The ``mem_test_max`` benchmark is commented out as it does not fit on any Stratix 10 device. This can be uncommented to run on an auto-sized device; however it may take a very long time to pack, place, and route.

.. seealso:: :ref:`titanium_benchmarks`
