.. _configurable_memory_block_example:

Configurable Memory Block Example
---------------------------------

A memory block with a reconfigurable aspect ratio.

.. code-block:: xml

    <pb_type name="memory" height="1">
        <input name="addr1" num_pins="14"/>
        <input name="addr2" num_pins="14"/>
        <input name="data" num_pins="16"/>
        <input name="we1" num_pins="1"/>
        <input name="we2" num_pins="1"/>
        <output name="out" num_pins="16"/>
        <clock name="clk" num_pins="1"/>

        <mode name="mem_1024x16_sp">
          <pb_type name="mem_1024x16_sp" blif_model=".subckt single_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr" num_pins="10" port_class="address"/>
            <input name="data" num_pins="16" port_class="data_in"/>
            <input name="we" num_pins="1" port_class="write_en"/>
            <output name="out" num_pins="16" port_class="data_out"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[9:0]" output="mem_1024x16_sp.addr">
            </direct>
            <direct name="data1" input="memory.data[15:0]" output="mem_1024x16_sp.data">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_1024x16_sp.we">
            </direct>
            <direct name="dataout1" input="mem_1024x16_sp.out" output="memory.out[15:0]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_1024x16_sp.clk">
            </direct>
          </interconnect>
        </mode>
        <mode name="mem_2048x8_dp">
          <pb_type name="mem_2048x8_dp" blif_model=".subckt dual_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr1" num_pins="11" port_class="address1"/>
            <input name="addr2" num_pins="11" port_class="address2"/>
            <input name="data1" num_pins="8" port_class="data_in1"/>
            <input name="data2" num_pins="8" port_class="data_in2"/>
            <input name="we1" num_pins="1" port_class="write_en1"/>
            <input name="we2" num_pins="1" port_class="write_en2"/>
            <output name="out1" num_pins="8" port_class="data_out1"/>
            <output name="out2" num_pins="8" port_class="data_out2"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[10:0]" output="mem_2048x8_dp.addr1">
            </direct>
            <direct name="address2" input="memory.addr2[10:0]" output="mem_2048x8_dp.addr2">
            </direct>
            <direct name="data1" input="memory.data[7:0]" output="mem_2048x8_dp.data1">
            </direct>
            <direct name="data2" input="memory.data[15:8]" output="mem_2048x8_dp.data2">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_2048x8_dp.we1">
            </direct>
            <direct name="writeen2" input="memory.we2" output="mem_2048x8_dp.we2">
            </direct>
            <direct name="dataout1" input="mem_2048x8_dp.out1" output="memory.out[7:0]">
            </direct>
            <direct name="dataout2" input="mem_2048x8_dp.out2" output="memory.out[15:8]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_2048x8_dp.clk">
            </direct>
          </interconnect>
        </mode>

        <mode name="mem_2048x8_sp">
          <pb_type name="mem_2048x8_sp" blif_model=".subckt single_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr" num_pins="11" port_class="address"/>
            <input name="data" num_pins="8" port_class="data_in"/>
            <input name="we" num_pins="1" port_class="write_en"/>
            <output name="out" num_pins="8" port_class="data_out"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[10:0]" output="mem_2048x8_sp.addr">
            </direct>
            <direct name="data1" input="memory.data[7:0]" output="mem_2048x8_sp.data">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_2048x8_sp.we">
            </direct>
            <direct name="dataout1" input="mem_2048x8_sp.out" output="memory.out[7:0]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_2048x8_sp.clk">
            </direct>
          </interconnect>
        </mode>
        <mode name="mem_4096x4_dp">
          <pb_type name="mem_4096x4_dp" blif_model=".subckt dual_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr1" num_pins="12" port_class="address1"/>
            <input name="addr2" num_pins="12" port_class="address2"/>
            <input name="data1" num_pins="4" port_class="data_in1"/>
            <input name="data2" num_pins="4" port_class="data_in2"/>
            <input name="we1" num_pins="1" port_class="write_en1"/>
            <input name="we2" num_pins="1" port_class="write_en2"/>
            <output name="out1" num_pins="4" port_class="data_out1"/>
            <output name="out2" num_pins="4" port_class="data_out2"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[11:0]" output="mem_4096x4_dp.addr1">
            </direct>
            <direct name="address2" input="memory.addr2[11:0]" output="mem_4096x4_dp.addr2">
            </direct>
            <direct name="data1" input="memory.data[3:0]" output="mem_4096x4_dp.data1">
            </direct>
            <direct name="data2" input="memory.data[7:4]" output="mem_4096x4_dp.data2">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_4096x4_dp.we1">
            </direct>
            <direct name="writeen2" input="memory.we2" output="mem_4096x4_dp.we2">
            </direct>
            <direct name="dataout1" input="mem_4096x4_dp.out1" output="memory.out[3:0]">
            </direct>
            <direct name="dataout2" input="mem_4096x4_dp.out2" output="memory.out[7:4]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_4096x4_dp.clk">
            </direct>
          </interconnect>
        </mode>

        <mode name="mem_4096x4_sp">
          <pb_type name="mem_4096x4_sp" blif_model=".subckt single_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr" num_pins="12" port_class="address"/>
            <input name="data" num_pins="4" port_class="data_in"/>
            <input name="we" num_pins="1" port_class="write_en"/>
            <output name="out" num_pins="4" port_class="data_out"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[11:0]" output="mem_4096x4_sp.addr">
            </direct>
            <direct name="data1" input="memory.data[3:0]" output="mem_4096x4_sp.data">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_4096x4_sp.we">
            </direct>
            <direct name="dataout1" input="mem_4096x4_sp.out" output="memory.out[3:0]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_4096x4_sp.clk">
            </direct>
          </interconnect>
        </mode>
        <mode name="mem_8192x2_dp">
          <pb_type name="mem_8192x2_dp" blif_model=".subckt dual_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr1" num_pins="13" port_class="address1"/>
            <input name="addr2" num_pins="13" port_class="address2"/>
            <input name="data1" num_pins="2" port_class="data_in1"/>
            <input name="data2" num_pins="2" port_class="data_in2"/>
            <input name="we1" num_pins="1" port_class="write_en1"/>
            <input name="we2" num_pins="1" port_class="write_en2"/>
            <output name="out1" num_pins="2" port_class="data_out1"/>
            <output name="out2" num_pins="2" port_class="data_out2"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[12:0]" output="mem_8192x2_dp.addr1">
            </direct>
            <direct name="address2" input="memory.addr2[12:0]" output="mem_8192x2_dp.addr2">
            </direct>
            <direct name="data1" input="memory.data[1:0]" output="mem_8192x2_dp.data1">
            </direct>
            <direct name="data2" input="memory.data[3:2]" output="mem_8192x2_dp.data2">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_8192x2_dp.we1">
            </direct>
            <direct name="writeen2" input="memory.we2" output="mem_8192x2_dp.we2">
            </direct>
            <direct name="dataout1" input="mem_8192x2_dp.out1" output="memory.out[1:0]">
            </direct>
            <direct name="dataout2" input="mem_8192x2_dp.out2" output="memory.out[3:2]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_8192x2_dp.clk">
            </direct>
          </interconnect>
        </mode>

        <mode name="mem_8192x2_sp">
          <pb_type name="mem_8192x2_sp" blif_model=".subckt single_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr" num_pins="13" port_class="address"/>
            <input name="data" num_pins="2" port_class="data_in"/>
            <input name="we" num_pins="1" port_class="write_en"/>
            <output name="out" num_pins="2" port_class="data_out"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[12:0]" output="mem_8192x2_sp.addr">
            </direct>
            <direct name="data1" input="memory.data[1:0]" output="mem_8192x2_sp.data">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_8192x2_sp.we">
            </direct>
            <direct name="dataout1" input="mem_8192x2_sp.out" output="memory.out[1:0]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_8192x2_sp.clk">
            </direct>
          </interconnect>
        </mode>
        <mode name="mem_16384x1_dp">
          <pb_type name="mem_16384x1_dp" blif_model=".subckt dual_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr1" num_pins="14" port_class="address1"/>
            <input name="addr2" num_pins="14" port_class="address2"/>
            <input name="data1" num_pins="1" port_class="data_in1"/>
            <input name="data2" num_pins="1" port_class="data_in2"/>
            <input name="we1" num_pins="1" port_class="write_en1"/>
            <input name="we2" num_pins="1" port_class="write_en2"/>
            <output name="out1" num_pins="1" port_class="data_out1"/>
            <output name="out2" num_pins="1" port_class="data_out2"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[13:0]" output="mem_16384x1_dp.addr1">
            </direct>
            <direct name="address2" input="memory.addr2[13:0]" output="mem_16384x1_dp.addr2">
            </direct>
            <direct name="data1" input="memory.data[0:0]" output="mem_16384x1_dp.data1">
            </direct>
            <direct name="data2" input="memory.data[1:1]" output="mem_16384x1_dp.data2">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_16384x1_dp.we1">
            </direct>
            <direct name="writeen2" input="memory.we2" output="mem_16384x1_dp.we2">
            </direct>
            <direct name="dataout1" input="mem_16384x1_dp.out1" output="memory.out[0:0]">
            </direct>
            <direct name="dataout2" input="mem_16384x1_dp.out2" output="memory.out[1:1]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_16384x1_dp.clk">
            </direct>
          </interconnect>
        </mode>

        <mode name="mem_16384x1_sp">
          <pb_type name="mem_16384x1_sp" blif_model=".subckt single_port_ram" class="memory" num_pb="1" area="1000">
            <input name="addr" num_pins="14" port_class="address"/>
            <input name="data" num_pins="1" port_class="data_in"/>
            <input name="we" num_pins="1" port_class="write_en"/>
            <output name="out" num_pins="1" port_class="data_out"/>
            <clock name="clk" num_pins="1" port_class="clock"/>
          </pb_type>
          <interconnect>
            <direct name="address1" input="memory.addr1[13:0]" output="mem_16384x1_sp.addr">
            </direct>
            <direct name="data1" input="memory.data[0:0]" output="mem_16384x1_sp.data">
            </direct>
            <direct name="writeen1" input="memory.we1" output="mem_16384x1_sp.we">
            </direct>
            <direct name="dataout1" input="mem_16384x1_sp.out" output="memory.out[0:0]">
            </direct>
            <direct name="clk" input="memory.clk" output="mem_16384x1_sp.clk">
            </direct>
          </interconnect>
        </mode>



      <fc_in type="frac"> 0.15</fc_in>
      <fc_out type="frac"> 0.125</fc_out>
      <pinlocations pattern="spread"/>
      <gridlocations>
        <loc type="col" start="2" repeat="5" priority="2"/>
      </gridlocations>
    </pb_type>


