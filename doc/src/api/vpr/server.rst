========
Server
========

server::update
--------------

.. doxygenfunction:: server::update
   :project: vpr

server::GateIO
--------------

.. doxygenclass:: server::GateIO
   :project: vpr
   :members:

server::Task
------------

.. doxygenfile:: commcmd.h
   :project: vpr

.. doxygenclass:: server::Task
   :project: vpr
   :members:

server::TaskResolver
--------------------

.. doxygenclass:: server::TaskResolver
   :project: vpr
   :members:

.. doxygenstruct:: server::CritPathsResult 
   :project: vpr
   :members:

.. doxygenfunction:: server::calc_critical_path
   :project: vpr

.. doxygenenum:: e_timing_report_detail
   :project: vpr

comm::Telegram
--------------

.. doxygenclass:: comm::TelegramHeader
   :project: vpr
   :members:

.. doxygenstruct:: comm::TelegramFrame
   :project: vpr
   :members:

.. doxygenclass:: comm::TelegramBuffer
   :project: vpr
   :members:

.. doxygenclass:: comm::ByteArray
   :project: vpr
   :members:

Parsers
-------

.. doxygenclass:: server::TelegramOptions
   :project: vpr
   :members:

.. doxygenclass:: comm::TelegramParser
   :project: vpr
   :members:


Compression utils
-----------------

.. doxygenfunction:: try_compress
   :project: vpr

.. doxygenfunction:: try_decompress
   :project: vpr
