Documenting VTR Code with Doxygen
=================================

VTR uses Doxygen and Sphinx for generating code documentation. Doxygen is used
to parse a codebase, extract code comments, and save them into an XML file.
The XML is then read by the Sphinx Breathe plugin, which converts it
to an HTML available publicly in the project documentation. The documentation
generated with Sphinx can be found in the API Reference section.

This note presents how to document source code in the VTR project
and check whether Doxygen can parse the created description. Code
conventions presented below were chosen arbitrarily for the project,
from many more options available in Doxygen. To read more about the tool,
visit the official `Doxygen documentation`_.

Documenting Code
----------------

There are three basic types of Doxygen code comments used in the VTR documentation:

- block comments
- one-line comments before a code element
- one-line comments after an element member

In most cases, a piece of documentation should be placed before a code
element. Comments after an element should be used only for documenting
members of enumeration types, structures, and unions.

Block Comments
++++++++++++++

You should use Doxygen block comments with both brief and detailed
descriptions to document code by default. As the name suggests, a brief
description should be a one-liner with general information about
the code element. A detailed description provides more specific
information about the element, its usage, or implementation details.
In the case of functions and methods, information about parameters and
returned value comes after the detailed description. Note that brief
and detailed descriptions have to be separated with at least one empty line.

Here is an example of a Doxygen block comment:

.. code-block:: c

   /**
    * @brief This is a brief function description
    *
    * This is a detailed description. It should be separated from
    * the brief description with one blank line.
    *
    *   @param a    A description of a
    *   @param b    A description of b
    *
    *   @return     A return value description
    */
   int my_func(int a, int b) {
       return a + b;
   }

General guidelines for using Doxygen block comments:

#. A block-comment block **has to** start with the ``/**``, otherwise
   Doxygen will not recognize it. All the comment lines **have to** be
   preceded by an asterisk. All the asterisks **have to** create a straight
   vertical line.

#. Brief and detailed descriptions **have to** be separated with one
   empty line.

#. A detailed description and a parameter list **should be** separated with
   one empty line.

#. A parameter list **should be** indented one level. All the parameter
   descriptions should be aligned together.

#. A returned value description should be separated with one empty line
   from either a detailed or a parameter description.

One-line Comments Before an Element
++++++++++++++++++++++++++++++++++++

One-line comments can be used instead of the block comments described above,
only if a brief description is sufficient for documenting the particular code
element. Usually, this is the case with global variables and defines.

Here is an example of a one-line Doxygen comment (before a code element):

.. code-block:: c

  /// @brief This is one-line documentation comment
  int var = 0;

General guidelines for using Doxygen one-line comments (before a code element):

#. A one-line comment before an element **has to** start with ``///``,
   otherwise Doxygen will not recognize it.

#. Since this style of code comments **should be** used only for
   brief descriptions, it **should** contain a ``@brief`` tag.

#. One-line comments **should not** be overused. They are acceptable for
   single variables and defines, but more complicated elements like classes and
   structures should be documented more carefully with Doxygen block
   comments.

One-line Comments After an Element Member
++++++++++++++++++++++++++++++++++++++++++

There is another type of one-line code comments used to document members
of enumeration types, structures, and unions. In those situations, the whole
element should be documented in a standard way using a Doxygen block comment.
However, the particular element members should be described after
they appear in the code with the one-line comments.

Here is an example of a one-line Doxygen comment (after an element member):

.. code-block:: c

   /**
    * @brief This is a brief enum description
    *
    * This is a detailed description. It should be separated from
    * the brief description with one blank line
    */
   enum seasons {
       spring = 3, ///< Describes spring enum value
       summer,     ///< Describes summer enum value
       autumn = 7, ///< Describes autumn enum value
       winter      ///< Describes winter enum value
   };

General guidelines for using Doxygen one-line comments (after an element member):

#. One-line code comment after an element member **has to** start with
   ``///<``, otherwise Doxygen will not recognize it.

#. This comment style **should be** used together with a Doxygen block
   comment for describing the whole element, before the members' description.

Documenting Files
-----------------

All files that contain the source code should be documented with
a Doxygen-style header. The file description in Doxygen is similar to
code element description, and should be placed at the beginning of the file.
The comment should contain information about an author, date of the document
creation, and a description of functionalities introduced in the file.

Here is an example of file documentation:

.. code-block:: c

   /**
    * @file
    * @author  John Doe
    * @date    2020-09-03
    * @brief   This is a brief document description
    *
    * This is a detailed description. It should be separated from
    * the brief description with one blank line
    */

General suggestions about a Doxygen file comments:

#. A file comment **has to** start with the ``@file`` tag,
   otherwise it will not be recognized by Doxygen.

#. The ``@file``, ``@author``, ``@date``, and ``@brief`` tags **should** form
   a single group of elements. A detailed description (if available)
   **has to** be placed one empty line after the brief description.

#. A file comment **should** consist of at least the ``@file`` and ``@brief``
   tags.

Validation of Doxygen Comments (Updating API Reference)
-------------------------------------------------------

Validation of Doxygen code comments might be time-consuming since it
requires setting the whole Doxygen project using Doxygen configuration
files (doxyfiles). One solution to that problem is to use the configuration
created for generating the official VTR documentation. The following steps
will show you how to add new code comments to the Sphinx API Reference,
available in the VTR documentation:

#. Ensure that the documented project has a doxyfile, and it is added to
   breathe configuration. All the doxyfiles used by the Sphinx documentation
   are placed in ``<vtr_root>/doc/_doxygen`` (For details, check
   :doc:`Sphinx API Documentation for C/C++ Projects <c_api_doc>`)
   This will ensure that Doxygen XMLs will be created for that project
   during the Sphinx documentation building process.

#. Check that the ``<vtr_root>/doc/src/api/<project_name>`` directory with
   a ``index.rst`` file exists. If not, create both the directory and the
   index file. Here is an example of the ``index.rst`` file for the VPR project.

   .. code-block:: rst

      VPR API
      =======

      .. toctree::
         :maxdepth: 1

         contexts
         netlist

   .. note::

      Do not forget about adding the index file title. The ``====`` marks
      should be of the same length as the title.

#. Create a RST file, which will contain the references to the Doxygen
   code comments. Sphinx uses the Breathe plugin for extracting Doxygen
   comments from the generated XML files. The simplest check can be done by
   dumping all the Doxygen comments from the single file with
   a ``..doxygenfile ::`` directive.

   Assuming that your RST file name is ``myrst.rst``, and you created it to check
   the Doxygen comments in the ``mycode.cpp`` file within the ``vpr`` project,
   the contents of the file might be the following:

   .. code-block:: rst

      =====
      MyRST
      =====

      .. doxygenfile:: mycode.cpp
         :project: vpr

   .. note::

      A complete list of Breathe directives can be found in the
      `Breathe documentation`_

#. Add the newly created RST file to the ``index.rst``. In this example, that
   will lead to the following change in the ``index.rst``:

   .. code-block:: rst

      VPR API
      =======

      .. toctree::
         :maxdepth: 1

         contexts
         netlist
         myrst

#. Generate the Sphinx documentation by using ``make html`` command inside
   the ``<vtr_root>/doc/`` directory.

#. The new section should be available in the API Reference. To verify that
   open the ``<vtr_root>/doc/_build/html/index.html`` with your browser and
   check the API Reference section. If the introduced code comments are
   unavailable, you can analyze the Sphinx build log.

Additional Resources
--------------------

- `Doxygen documentation`_
- `Breathe documentation`_

.. _Breathe documentation: https://breathe.readthedocs.io/en/latest/
.. _Doxygen documentation: https://www.doxygen.nl/index.html
