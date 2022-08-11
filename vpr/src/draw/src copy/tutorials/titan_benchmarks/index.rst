.. _titan_benchmarks_tutorial:

Running the Titan Benchmarks
----------------------------

This tutorial describes how to run the :ref:`Titan benchmarks <titan_benchmarks>` with VTR.

Integrating the Titan benchmarks into VTR
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Titan benchmarks take up a large amount of disk space and are not distributed directly with VTR.

The Titan benchmarks can be automatically integrated into the VTR source tree by running the following from the root of the VTR source tree:

.. code-block:: console

    $ make get_titan_benchmarks

which downloads and extracts the benchmarks into the VTR source tree:

.. code-block:: console

    Warning: A typical Titan release is a ~1GB download, and uncompresses to ~10GB.
    Starting download in 15 seconds...
    Downloading http://www.eecg.utoronto.ca/~kmurray/titan/titan_release_1.1.0.tar.gz
    .....................................................................................................
    Downloading http://www.eecg.utoronto.ca/~kmurray/titan/titan_release_1.1.0.md5
    Verifying checksum
    OK
    Searching release for benchmarks and architectures...
    Extracting titan_release_1.1.0/benchmarks/titan23/sparcT2_core/netlists/sparcT2_core_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/sparcT2_core_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/LU230/netlists/LU230_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/LU230_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/segmentation/netlists/segmentation_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/segmentation_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/openCV/netlists/openCV_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/openCV_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/bitcoin_miner/netlists/bitcoin_miner_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/bitcoin_miner_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/sparcT1_chip2/netlists/sparcT1_chip2_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/sparcT1_chip2_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/mes_noc/netlists/mes_noc_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/mes_noc_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/bitonic_mesh/netlists/bitonic_mesh_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/bitonic_mesh_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/dart/netlists/dart_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/dart_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/cholesky_bdti/netlists/cholesky_bdti_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/cholesky_bdti_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/stereo_vision/netlists/stereo_vision_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/stereo_vision_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/neuron/netlists/neuron_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/neuron_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/gaussianblur/netlists/gaussianblur_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/gaussianblur_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/gsm_switch/netlists/gsm_switch_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/gsm_switch_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/sparcT1_core/netlists/sparcT1_core_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/sparcT1_core_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/des90/netlists/des90_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/des90_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/LU_Network/netlists/LU_Network_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/LU_Network_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/denoise/netlists/denoise_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/denoise_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/stap_qrd/netlists/stap_qrd_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/stap_qrd_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/directrf/netlists/directrf_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/directrf_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/SLAM_spheric/netlists/SLAM_spheric_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/SLAM_spheric_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/minres/netlists/minres_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/minres_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/benchmarks/titan23/cholesky_mc/netlists/cholesky_mc_stratixiv_arch_timing.blif to ./vtr_flow/benchmarks/titan_blif/cholesky_mc_stratixiv_arch_timing.blif
    Extracting titan_release_1.1.0/arch/stratixiv_arch.timing.no_pack_patterns.xml to ./vtr_flow/arch/titan/stratixiv_arch.timing.no_pack_patterns.xml
    Extracting titan_release_1.1.0/arch/stratixiv_arch.timing.xml to ./vtr_flow/arch/titan/stratixiv_arch.timing.xml
    Extracting titan_release_1.1.0/arch/stratixiv_arch.timing.no_directlink.xml to ./vtr_flow/arch/titan/stratixiv_arch.timing.no_directlink.xml
    Extracting titan_release_1.1.0/arch/stratixiv_arch.timing.no_chain.xml to ./vtr_flow/arch/titan/stratixiv_arch.timing.no_chain.xml
    Done
    Titan architectures: vtr_flow/arch/titan
    Titan benchmarks: vtr_flow/benchmarks/titan_blif

Once completed all the Titan benchmark BLIF netlists can be found under ``$VTR_ROOT/vtr_flow/benchmarks/titan_blif``, and the Titan architectures under ``$VTR_ROOT/vtr_flow/arch/titan``.

.. note:: :term:`$VTR_ROOT` corresponds to the root of the VTR source tree.

Running benchmarks manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the benchmarks have been integrated into VTR they can be run manually.

For example, the follow uses :ref:`VPR` to implement the ``neuron`` benchmark onto the ``startixiv_arch.timing.xml`` architecture at a :option:`channel width <vpr --route_chan_width>` of 300 tracks:

.. code-block:: console

    $ vpr $VTR_ROOT/vtr_flow/arch/titan/stratixiv_arch.timing.xml $VTR_ROOT/vtr_flow/benchmarks/titan_blif/neuron_stratixiv_arch_timing.blif --route_chan_width 300
