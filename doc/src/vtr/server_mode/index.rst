.. _server_mode:

Server Mode
================

If VPR is in server mode, it listens on a socket for commands from a client. Currently, this is used to enable interactive timing analysis and visualization of timing paths in the VPR UI under the control of a separate client.
VPR provides the ability to run in server mode using the following command-line arguments.

.. code-block:: none

  --server --port_num 60555

Server mode may only accept a single client application connection and respond to two types of requests: **get critical path report** and **highlight selected critical path elements**.

Communication telegram
-------------------------

Telegram consists of two parts: a fixed-size **telegram header** and a variable-size **telegram body**.
The telegram header contains helper information required to properly extract the telegram body sequence from the data flow.

.. _fig_comm_telegram_structure:

.. figure:: comm_telegram_structure.*

    Communication telegram structure.

.. note:: The telegram body itself could be compressed with zlib to minimize the amount of data transferred over the socket.
  This compression is applied to the response of the 'get critical path report' request. The compressor ID byte in the telegram header signals whether the telegram body is compressed.
  When the compressor ID is null, the telegram body is not compressed. If the compressor ID is 'z', it means the body is compressed with zlib.

.. note:: The checksum field contains the telegram body checksum. This checksum is used to validate the consistency of the telegram body during the dispatching phase.
  If checksums are mismatched, the telegram is considered invalid and is skipped in processing.


.. _fig_comm_telegram_body_structure:

.. figure:: comm_telegram_body_structure.*

    Communication telegram body structure.

    **telegram body** is a flat **JSON** structure.

    **CMD** could have following integer values:

    - 0 - command id for **get critical path**
    - 1 - command id for **highlight selected path elements**

    JOB_ID is a unique ID for a task. It is used to associate the request with the response by matching the same JOB_ID. Each new client request should increment the JOB_ID value; otherwise, it will not be clear which request the current response belongs to.


Get critical path timing report example
---------------------------------------

  Let's take a look at an example of a request timing report telegram body with the following options:

  - path_num = 1
  - path_type = "setup"
  - details_level = "netlist"
  - is_flat_routing = false

  .. code-block:: json
      :caption: telegram body **REQUEST** example
      :linenos:

      {
        "JOB_ID": "1",
        "CMD": "0",
        "OPTIONS": "int:path_num:1;string:path_type:setup;string:details_level:netlist;bool:is_flat_routing:0"
      }

  **path_type** could have following string values:

  - "setup"
  - "hold"

  **details_level** could have following string values:

  - "netlist"
  - "aggregated"
  - "detailed"
  - "debug"

  Response will look like:

  .. code-block:: json
      :caption: telegram body **RESPONSE** example
      :linenos:

      {
        "JOB_ID": "1",
        "CMD": "0",
        "OPTIONS": "int:path_num:1;string:path_type:setup;string:details_level:netlist;bool:is_flat_routing:0",
        "DATA": "
          #Timing report of worst 1 path(s)
          # Unit scale: 1e-09 seconds
          # Output precision: 3

          #Path 1
          Startpoint: count[1].Q[0] (dffsre clocked by clk)
          Endpoint  : count[13].D[0] (dffsre clocked by clk)
          Path Type : setup

          Point                                                                       Incr      Path
          ------------------------------------------------------------------------------------------
          clock clk (rise edge)                                                      0.000     0.000
          clock source latency                                                       0.000     0.000
          clk.inpad[0] (.input)                                                      0.000     0.000
          count[1].C[0] (dffsre)                                                     0.715     0.715
          count[1].Q[0] (dffsre) [clock-to-output]                                   0.286     1.001
          count_adder_carry_p_cout[2].p[0] (adder_carry)                             0.573     1.574
          count_adder_carry_p_cout[2].cout[0] (adder_carry)                          0.068     1.642
          count_adder_carry_p_cout[3].cin[0] (adder_carry)                           0.043     1.685
          count_adder_carry_p_cout[3].cout[0] (adder_carry)                          0.070     1.755
          count_adder_carry_p_cout[4].cin[0] (adder_carry)                           0.053     1.808
          count_adder_carry_p_cout[4].cout[0] (adder_carry)                          0.070     1.877
          count_adder_carry_p_cout[5].cin[0] (adder_carry)                           0.043     1.921
          count_adder_carry_p_cout[5].cout[0] (adder_carry)                          0.070     1.990
          count_adder_carry_p_cout[6].cin[0] (adder_carry)                           0.053     2.043
          count_adder_carry_p_cout[6].cout[0] (adder_carry)                          0.070     2.113
          count_adder_carry_p_cout[7].cin[0] (adder_carry)                           0.043     2.156
          count_adder_carry_p_cout[7].cout[0] (adder_carry)                          0.070     2.226
          count_adder_carry_p_cout[8].cin[0] (adder_carry)                           0.053     2.279
          count_adder_carry_p_cout[8].cout[0] (adder_carry)                          0.070     2.348
          count_adder_carry_p_cout[9].cin[0] (adder_carry)                           0.043     2.391
          count_adder_carry_p_cout[9].cout[0] (adder_carry)                          0.070     2.461
          count_adder_carry_p_cout[10].cin[0] (adder_carry)                          0.053     2.514
          count_adder_carry_p_cout[10].cout[0] (adder_carry)                         0.070     2.584
          count_adder_carry_p_cout[11].cin[0] (adder_carry)                          0.043     2.627
          count_adder_carry_p_cout[11].cout[0] (adder_carry)                         0.070     2.696
          count_adder_carry_p_cout[12].cin[0] (adder_carry)                          0.053     2.749
          count_adder_carry_p_cout[12].cout[0] (adder_carry)                         0.070     2.819
          count_adder_carry_p_cout[13].cin[0] (adder_carry)                          0.043     2.862
          count_adder_carry_p_cout[13].cout[0] (adder_carry)                         0.070     2.932
          count_adder_carry_p_cout[14].cin[0] (adder_carry)                          0.053     2.985
          count_adder_carry_p_cout[14].sumout[0] (adder_carry)                       0.040     3.025
          count_dffsre_Q_D[13].in[0] (.names)                                        0.564     3.589
          count_dffsre_Q_D[13].out[0] (.names)                                       0.228     3.818
          count[13].D[0] (dffsre)                                                    0.000     3.818
          data arrival time                                                                    3.818

          clock clk (rise edge)                                                      0.000     0.000
          clock source latency                                                       0.000     0.000
          clk.inpad[0] (.input)                                                      0.000     0.000
          count[13].C[0] (dffsre)                                                    0.715     0.715
          clock uncertainty                                                          0.000     0.715
          cell setup time                                                           -0.057     0.659
          data required time                                                                   0.659
          ------------------------------------------------------------------------------------------
          data required time                                                                   0.659
          data arrival time                                                                   -3.818
          ------------------------------------------------------------------------------------------
          slack (VIOLATED)                                                                    -3.159


          #End of timing report
          #RPT METADATA:
          path_index/clock_launch_path_elements_num/arrival_path_elements_num
          0/2/30
      ",
        "STATUS": "1"
      }

Draw selected critical path elements example
--------------------------------------------

  Let's take a look at an example of a request timing report telegram body with the following options:

  - path_elements = path 0 and it's sub-elements 10,11,12,13,14,15,20,21,22,23,24,25
  - high_light_mode = "crit path flylines delays"
  - draw_path_contour = 1

  .. code-block:: json
      :caption: telegram body **REQUEST** example
      :linenos:

      {
        "JOB_ID": "2",
        "CMD": "1",
        "OPTIONS": "string:path_elements:0#10,11,12,13,14,15,20,21,22,23,24,25;string:high_light_mode:crit path flylines delays;bool:draw_path_contour:1"
      }

  **high_light_mode** could have following string values:

  - "crit path flylines"
  - "crit path flylines delays"
  - "crit path routing"
  - "crit path routing delays"

  Response will look like:

  .. code-block:: json
      :caption: telegram body **RESPONSE** example
      :linenos:

      {
        "JOB_ID": "2",
        "CMD": "1",
        "OPTIONS": "string:path_elements:0#10,11,12,13,14,15,20,21,22,23,24,25;string:high_light_mode:crit path flylines delays;bool:draw_path_contour:1",
        "DATA": "",
        "STATUS": "1"
      }

  .. note:: If status is not 1, the field ***DATA*** contains error string.





