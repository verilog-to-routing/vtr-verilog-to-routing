/********************************************************************************
 * 		REGRESSION TEST INFRASTRUCTURE DOCUMENTATION			*
 *******************************************************************************/

/* There are currently four levels of regression testing as described below. 
 * @nooruddin
 * @17/05/12

/***************
 *  LEVEL ONE  *
 **************/

/* Basic VTR Regression - "vtr_reg_basic"
 * MANDATORY - Must be run by ALL developers before committing. 

Architecture/Benchmark:
	- k4_N10_memSize16384_memData64.xml/ch_intrinsics.v
					.../diffeq1.v
	- k6_N10_memDepth16384_memData64_40nm_nofrac_timing.xml/ch_intrinsics.v
							.../diff_eq1.v

Estimated Runtime: -

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

/* Basic VPR Regression - "vpr_reg_basic"
 * MANDATORY - Must be run by ALL developers before committing ONLY if the 
 * basic VTR regression was not carried out.

Architecture/Benchmark
	- IPR

	
/*************
 * LEVEL TWO *
 ************/

/* Strong VTR Regression - "vtr_reg_strong"
 * OPTIONAL - Can be run by developers before committing. 

Architecture/Benchmark:
	- k4_N10_memSize524288_memData64.xml/ch_intrinsics.v
	- k6_N10_memDepth16384_memData64_40nm_timing.xml/ch_intrinsics.v
	- k6_N8_I80_fleI10_fleO2_ff2_nmodes_2.xml/ch_intrinsics.v
	- k6_N10_soft_logic_only.xml/diffeq.pre-vpr.blif
	- hard_fpu_arch_timing.xml/mm3.v

Estimated Runtime: -

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt


/***************
 * LEVEL THREE *
 **************/

/* Nightly VTR Regression - "vtr_reg_nightly"
 * To be run by automated build system every night.

Architecture/Benchmark:
	- vtr_reg_qor_small

Estimated Runtime: -

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt


/**************
 * LEVEL FOUR *
 *************/

/* Weekly VTR Regression - "vtr_reg_weekly"
 * To be run by automated build system every weekend.

Architecture/Benchmark:
	- vtr_reg_fpu_hard_block_arch
	- vtr_reg_qor

Estimated Runtime: -

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt
