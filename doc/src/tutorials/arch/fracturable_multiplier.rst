.. _fracturable_multiplier_example:

Fracturable Multiplier Example
------------------------------

A 36x36 multiplier fracturable into 18x18s and 9x9s

.. code-block:: xml

    <pb_type name="mult_36" height="3">
        <input name="a" num_pins="36"/>
        <input name="b" num_pins="36"/>
        <output name="out" num_pins="72"/>

        <mode name="two_divisible_mult_18x18">
          <pb_type name="divisible_mult_18x18" num_pb="2">
            <input name="a" num_pins="18"/>
            <input name="b" num_pins="18"/>
            <output name="out" num_pins="36"/>

            <mode name="two_mult_9x9">
              <pb_type name="mult_9x9_slice" num_pb="2">
                <input name="A_cfg" num_pins="9"/>
                <input name="B_cfg" num_pins="9"/>
                <output name="OUT_cfg" num_pins="18"/>

                <pb_type name="mult_9x9" blif_model=".subckt multiply" num_pb="1" area="300">
                  <input name="a" num_pins="9"/>
                  <input name="b" num_pins="9"/>
                  <output name="out" num_pins="18"/>
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="{a b}" out_port="out"/>
                </pb_type>

                <interconnect>
                  <direct name="a2a" input="mult_9x9_slice.A_cfg" output="mult_9x9.a">
                    <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_9x9_slice.A_cfg" out_port="mult_9x9.a"/>
                    <C_constant C="1.89e-13" in_port="mult_9x9_slice.A_cfg" out_port="mult_9x9.a"/>
                  </direct>
                  <direct name="b2b" input="mult_9x9_slice.B_cfg" output="mult_9x9.b">
                    <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_9x9_slice.B_cfg" out_port="mult_9x9.b"/>
                    <C_constant C="1.89e-13" in_port="mult_9x9_slice.B_cfg" out_port="mult_9x9.b"/>
                  </direct>
                  <direct name="out2out" input="mult_9x9.out" output="mult_9x9_slice.OUT_cfg">
                    <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_9x9.out" out_port="mult_9x9_slice.OUT_cfg"/>
                    <C_constant C="1.89e-13" in_port="mult_9x9.out" out_port="mult_9x9_slice.OUT_cfg"/>
                  </direct>
                </interconnect>
              </pb_type>
              <interconnect>
                <direct name="a2a" input="divisible_mult_18x18.a" output="mult_9x9_slice[1:0].A_cfg">
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="divisible_mult_18x18.a" out_port="mult_9x9_slice[1:0].A_cfg"/>
                  <C_constant C="1.89e-13" in_port="divisible_mult_18x18.a" out_port="mult_9x9_slice[1:0].A_cfg"/>
                </direct>
                <direct name="b2b" input="divisible_mult_18x18.b" output="mult_9x9_slice[1:0].B_cfg">
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="divisible_mult_18x18.b" out_port="mult_9x9_slice[1:0].B_cfg"/>
                  <C_constant C="1.89e-13" in_port="divisible_mult_18x18.b" out_port="mult_9x9_slice[1:0].B_cfg"/>
                </direct>
                <direct name="out2out" input="mult_9x9_slice[1:0].OUT_cfg" output="divisible_mult_18x18.out">
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_9x9_slice[1:0].OUT_cfg" out_port ="divisible_mult_18x18.out"/>
                  <C_constant C="1.89e-13" in_port="mult_9x9_slice[1:0].OUT_cfg" out_port="divisible_mult_18x18.out"/>
                </direct>
              </interconnect>
            </mode>

            <mode name="mult_18x18">
              <pb_type name="mult_18x18_slice" num_pb="1">
                <input name="A_cfg" num_pins="18"/>
                <input name="B_cfg" num_pins="18"/>
                <output name="OUT_cfg" num_pins="36"/>

                <pb_type name="mult_18x18" blif_model=".subckt multiply" num_pb="1"  area="1000">
                  <input name="a" num_pins="18"/>
                  <input name="b" num_pins="18"/>
                  <output name="out" num_pins="36"/>
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="{a b}" out_port="out"/>
                </pb_type>

                <interconnect>
                  <direct name="a2a" input="mult_18x18_slice.A_cfg" output="mult_18x18.a">
                    <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_18x18_slice.A_cfg" out_port="mult_18x18.a"/>
                    <C_constant C="1.89e-13" in_port="mult_18x18_slice.A_cfg" out_port="mult_18x18.a"/>
                  </direct>
                  <direct name="b2b" input="mult_18x18_slice.B_cfg" output="mult_18x18.b">
                    <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_18x18_slice.B_cfg" out_port="mult_18x18.b"/>
                    <C_constant C="1.89e-13" in_port="mult_18x18_slice.B_cfg" out_port="mult_18x18.b"/>
                  </direct>
                  <direct name="out2out" input="mult_18x18.out" output="mult_18x18_slice.OUT_cfg">
                    <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_18x18.out" out_port="mult_18x18_slice.OUT_cfg"/>
                    <C_constant C="1.89e-13" in_port="mult_18x18.out" out_port="mult_18x18_slice.OUT_cfg"/>
                  </direct>
                </interconnect>
              </pb_type>
              <interconnect>
                <direct name="a2a" input="divisible_mult_18x18.a" output="mult_18x18_slice.A_cfg">
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="divisible_mult_18x18.a" out_port="mult_18x18_slice.A_cfg"/>
                  <C_constant C="1.89e-13" in_port="divisible_mult_18x18.a" out_port="mult_18x18_slice.A_cfg"/>
                </direct>
                <direct name="b2b" input="divisible_mult_18x18.b" output="mult_18x18_slice.B_cfg">
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="divisible_mult_18x18.b" out_port="mult_18x18_slice.B_cfg"/>
                  <C_constant C="1.89e-13" in_port="divisible_mult_18x18.b" out_port="mult_18x18_slice.B_cfg"/>
                </direct>
                <direct name="out2out" input="mult_18x18_slice.OUT_cfg" output="divisible_mult_18x18.out">
                  <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_18x18_slice.OUT_cfg" out_port="divisible_mult_18x18.out"/>
                  <C_constant C="1.89e-13" in_port="mult_18x18_slice.OUT_cfg" out_port="divisible_mult_18x18.out"/>
                </direct>
              </interconnect>
            </mode>
          </pb_type>
          <interconnect>
            <direct name="a2a" input="mult_36.a" output="divisible_mult_18x18[1:0].a">
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36.a" out_port="divisible_mult_18x18[1:0].a"/>
              <C_constant C="1.89e-13" in_port="mult_36.a" out_port="divisible_mult_18x18[1:0].a"/>
            </direct>
            <direct name="b2b" input="mult_36.b" output="divisible_mult_18x18[1:0].a">
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36.b" out_port="divisible_mult_18x18[1:0].a"/>
              <C_constant C="1.89e-13" in_port="mult_36.b" out_port="divisible_mult_18x18[1:0].a"/>
            </direct>
            <direct name="out2out" input="divisible_mult_18x18[1:0].out" output="mult_36.out">
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="divisible_mult_18x18[1:0].out" out_port ="mult_36.out"/>
              <C_constant C="1.89e-13" in_port="divisible_mult_18x18[1:0].out" out_port="mult_36.out"/>
            </direct>
          </interconnect>
        </mode>

        <mode name="mult_36x36">
          <pb_type name="mult_36x36_slice" num_pb="1">
            <input name="A_cfg" num_pins="36"/>
            <input name="B_cfg" num_pins="36"/>
            <output name="OUT_cfg" num_pins="72"/>

            <pb_type name="mult_36x36" blif_model=".subckt multiply" num_pb="1" area="4000">
              <input name="a" num_pins="36"/>
              <input name="b" num_pins="36"/>
              <output name="out" num_pins="72"/>
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="{a b}" out_port="out"/>
            </pb_type>

            <interconnect>
              <direct name="a2a" input="mult_36x36_slice.A_cfg" output="mult_36x36.a">
                <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36x36_slice.A_cfg" out_port="mult_36x36.a"/>
                <C_constant C="1.89e-13" in_port="mult_36x36_slice.A_cfg" out_port="mult_36x36.a"/>
              </direct>
              <direct name="b2b" input="mult_36x36_slice.B_cfg" output="mult_36x36.b">
                <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36x36_slice.B_cfg" out_port="mult_36x36.b"/>
                <C_constant C="1.89e-13" in_port="mult_36x36_slice.B_cfg" out_port="mult_36x36.b"/>
              </direct>
              <direct name="out2out" input="mult_36x36.out" output="mult_36x36_slice.OUT_cfg">
                <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36x36.out" out_port="mult_36x36_slice.OUT_cfg"/>
                <C_constant C="1.89e-13" in_port="mult_36x36.out" out_port="mult_36x36_slice.OUT_cfg"/>
              </direct>
            </interconnect>
          </pb_type>
          <interconnect>
            <direct name="a2a" input="mult_36.a" output="mult_36x36_slice.A_cfg">
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36.a" out_port="mult_36x36_slice.A_cfg"/>
              <C_constant C="1.89e-13" in_port="mult_36.a" out_port="mult_36x36_slice.A_cfg"/>
            </direct>
            <direct name="b2b" input="mult_36.b" output="mult_36x36_slice.B_cfg">
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36.b" out_port="mult_36x36_slice.B_cfg"/>
              <C_constant C="1.89e-13" in_port="mult_36.b" out_port="mult_36x36_slice.B_cfg"/>
            </direct>
            <direct name="out2out" input="mult_36x36_slice.OUT_cfg" output="mult_36.out">
              <delay_constant max="2.03e-13" min="1.89e-13" in_port="mult_36x36_slice.OUT_cfg" out_port="mult_36.out"/>
              <C_constant C="1.89e-13" in_port="mult_36x36_slice.OUT_cfg" out_port="mult_36.out"/>
            </direct>
          </interconnect>
        </mode>



      <fc_in type="frac"> 0.15</fc_in>
      <fc_out type="frac"> 0.125</fc_out>
      <pinlocations pattern="spread"/>

      <gridlocations>
        <loc type="col" start="4" repeat="5" priority="2"/>
      </gridlocations>
    </pb_type>


