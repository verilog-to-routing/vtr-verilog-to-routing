Install VTR
-----------

#. :ref:`Download <get_vtr>` the VTR release
#. Unpack the release in a directory of your choice (herafter referred to as ``<vtr>``)
#. Navigate to ``<vtr>`` and run ::

    make

   which will build all the required tools.

The complete VTR flow has been tested on 64-bit Linux systems.
The flow should work in other platforms (32-bit Linux, Windows with cygwin) but this is untested.
Please let us know your experience with building VTR so that we can improve the experience for others.

The tools included in the complete VTR package have been tested for compatibility.
If you download a different version of those tools, then those versions may not be mutually compatible with the VTR release.

.. seealso:: For additional information on building VPR on other platforms see :ref:`compiling_vpr`

Verifying Installation
~~~~~~~~~~~~~~~~~~~~~~
To verfiy that VTR has been installed correctly run::

    <vtr>/vtr_flow/scripts/run_vtr_task.pl basic_flow

The expected output is::

    k6_N10_memSize16384_memData64_40nm_timing/ch_intrinsics...OK
