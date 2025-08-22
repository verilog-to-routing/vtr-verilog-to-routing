namespace eval ::slang {
	# INFO: Maps HDL files to respective top module(s) 
	# top_map insertion syntax:
	# <file> <top module>
	array set top_map {
		and_latch.v and_latch
		multiclock_output_and_latch.v multiclock_output_and_latch
		multiclock_reader_writer.v multiclock_reader_writer
		multiclock_separate_and_latch.v multiclock_separate_and_latch
		arm_core.v arm_core
		bgm.v bgm
		blob_merge.v RLE_BlobMerging
		boundtop.v paj_boundtop_hierarchy_no_mem
		ch_intrinsics.v memset
		ch_intrinsics_modified.v memset
		diffeq1.v diffeq_paj_convert
		diffeq2.v diffeq_f_systemC
		LU8PEEng.v LU8PEEng
		LU32PEEng.v LU32PEEng
		LU64PEEng.v LU64PEEng
		mcml.v mcml
		mkDelayWorker32B.v mkDelayWorker32B
		mkPktMerge.v mkPktMerge
		mkSMAdapter4B.v mkSMAdapter4B
		or1200.v or1200_flat
		raygentop.v paj_raygentop_hierarchy_no_mem
		sha.v sha1
		stereovision0.v sv_chip0_hierarchy_no_mem
		stereovision1.v sv_chip1_hierarchy_no_mem
		stereovision2.v sv_chip2_hierarchy_no_mem
		stereovision3.v sv_chip3_hierarchy_no_mem
		button_controller.sv top
		display_control.sv top
		debounce.sv top
		timer.sv top
		deepfreeze.style1.sv top
		pulse_led.v top
		clock.sv top
		single_ff.v top
		single_wire.v top
		PWM.v top
		flattened_pulse_width_led.sv top
		modify_count.sv top
		time_counter.sv top
		spree.v system
		attention_layer.v attention_layer
		bnn.v bnn
		tpu_like.small.os.v top
		tpu_like.small.ws.v top
		dla_like.small.v DLA
		conv_layer_hls.v top
		conv_layer.v conv_layer
		eltwise_layer.v eltwise_layer
		robot_rl.v robot_maze
		reduction_layer.v reduction_layer
		spmv.v spmv
		softmax.v softmax
		test.v top
		mult_9x9.v mult_nxn
		mult_8x8.v mult_nxn
		mult_7x7.v mult_nxn
		mult_6x6.v mult_nxn
		mult_5x5.v mult_nxn
		mult_4x4.v mult_nxn
		mm3.v mm3
		d_flip_flop.v d_flip_flop
	}
	# INFO: List of HDL includes files 
	# includes_map insertion syntax:
	# <file> include
	array set includes_map {
		hard_block_include.v include
		complex_dsp_include.v include
		hard_mem_include.v include
		generic_definitions1.vh include
		generic_definitions2.vh include
		memory_controller.v include
	}
}
