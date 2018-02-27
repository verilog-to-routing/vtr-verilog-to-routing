Install VTR
-----------

#. :ref:`Download <get_vtr>` the VTR release
#. Unpack the release in a directory of your choice (herafter referred to as :term:`$VTR_ROOT`)
#. Navigate to :term:`$VTR_ROOT` and run ::

    make

   which will build all the required tools.

.. warning:: :term:`$VTR_ROOT` should be replaced with the path to the root of VTR source tree on your machine.

The complete VTR flow has been tested on 64-bit Linux systems.
The flow should work in other platforms (32-bit Linux, Windows with cygwin) but this is untested.

.. seealso:: More information about building VTR can be found in the :ref:`developer_guide`

Please :ref:`let us know <contact>` your experience with building VTR so that we can improve the experience for others.

The tools included official VTR releases have been tested for compatibility.
If you download a different version of those tools, then those versions may not be mutually compatible with the VTR release.

Verifying Installation
~~~~~~~~~~~~~~~~~~~~~~
To verfiy that VTR has been installed correctly run::

    $VTR_ROOT/vtr_flow/scripts/run_vtr_task.pl basic_flow

The expected output is::

    k6_N10_memSize16384_memData64_40nm_timing/ch_intrinsics...OK
