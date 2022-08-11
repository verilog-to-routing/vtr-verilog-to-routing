===========================================
Sphinx API Documentation for C/C++ Projects
===========================================

The Sphinx API documentation for VTR C/C++ projects is created using
Doxygen and Breathe plugin. Doxygen is a standard tool for generating
documentation from annotated code. It is used to generate XML output that can
then be parsed by the Breathe plugin, which provides the RST directives used to
embed the code comments into the Sphinx documentation.

Every VPR C/C++ project requires a few steps that have to be completed,
to generate the Sphinx documentation:

  - Create doxyfile for the project
  - Update the Breathe config
  - Create RST files with the API description using Breathe directives
  - Generate the project documentation


Create Doxyfile
---------------

A doxyfile a Doxygen configuration file that provides all the necessary
information about the documented project. It is used to generate Doxygen
output in the chosen format.

The configuration includes the specification of input files, output directory,
generated documentation formats, and much more. The config for a particular
VPR project should be saved in the ``<vtr-verilog-to-routing>/doc/_doxygen``
directory. The doxyfile should be named as ``<key>.dox``, where ``<key>`` is
a ``breathe_projects`` dictionary key associated with the VPR project.

The minimal doxyfile should contain only the configuration
values that are not set by default. As mentioned before, the Breathe plugin
expects the XML input. Therefore the ``GENERATE_XML`` option should be turned on.
Below there is a content of ``vpr.dox`` file content, which contains
the VPR Doxygen configuration:

.. literalinclude:: ../../_doxygen/vpr.dox
   :name: vpr.dox

The general Doxyfile template can be generated using::

   doxygen -g template.dox


Breathe Configuration
---------------------

Breathe plugin is responsible for parsing the XML file generated
by the Doxygen. It provides the convenient RST directives that allow to
embed the read documentation into the Sphinx documentation.

To add the new project to the Sphinx API generation mechanism, you have to
update the ``breathe_projects`` dictionary in the Sphinx ``conf.py`` file.
The dictionary consist of key-value pairs which describe the project.
The key is related to the project name that will be used in the Breathe plugin
directives. The value associated with the key points to the directory where
the XML Doxygen output is generated.

Example of this configuration structure is presented below::

    breathe_projects = {
        "vpr"          : "../_build/doxygen/vpr/xml",
        "abc"          : "../_build/doxygen/abc/xml",
        "ace2"         : "../_build/doxygen/ace2/xml",
        "ODIN_II"      : "../_build/doxygen/ODIN_II/xml",
        "blifexplorer" : "../_build/doxygen/blifexplorer/xml",
    }

More information about the Breathe plugin can be found in the
`Breathe Documentation`_.


Create RST with API Documentation
---------------------------------

.. _Directives & Config Variables: https://breathe.readthedocs.io/en/latest/directives.html

To generate the Sphinx API documentation, you should use the directives
provided by the Breathe plugin. A complete list of Breathe directives
can be found in the `Directives & Config Variables`_ section of the
`Breathe Documentation`_.

Example of ``doxygenclass`` directive used for the VPR project is presented below::

   .. doxygenclass:: VprContext
      :project: vpr
      :members:


Generate the Documentation
--------------------------

Currently, the Doxygen is set up to run automatically whenever
the documentation is regenerated. The Doxygen XML generation is skipped
when the Doxygen is not installed on your machine or when the
``SKIP_DOXYGEN=True`` environment variable is set.

The Doxygen is being run for every project described in
the ``breathe_projects`` dictionary. Therefore it is essential to keep
the same name of the project name key and the doxyfile name.

.. _Breathe Documentation: https://breathe.readthedocs.io/en/latest/
.. _Directives & Config Variables: https://breathe.readthedocs.io/en/latest/directives.html
