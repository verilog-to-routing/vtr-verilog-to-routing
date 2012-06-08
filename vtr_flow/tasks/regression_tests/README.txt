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

Estimated Runtime: ~2-5 minutes

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
				./run_reg_test.pl vtr_reg_basic
To create golden results, use: 
				./run_reg_test.pl -create_golden vtr_reg_basic

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

Create golden results with:
	<scripts_path>/parse_vtr_task.pl -create_golden -l <tasks_path>/regression_tests/vtr_reg_basic/task_list.txt

	
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

Estimated Runtime: ~5-10 minutes

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
				./run_reg_test.pl vtr_reg_strong
To create golden results, use: 
				./run_reg_test.pl -create_golden vtr_reg_strong

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt

Create golden results with:
	<scripts_path>/parse_vtr_task.pl -create_golden -l <tasks_path>/regression_tests/vtr_reg_strong/task_list.txt


/***************
 * LEVEL THREE *
 **************/

/* Nightly VTR Regression - "vtr_reg_nightly"
 * To be run by automated build system every night.

Architecture/Benchmark:
	- vtr_reg_qor_small

Estimated Runtime: ~3-5 hours

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
				./run_reg_test.pl vtr_reg_nightly
To create golden results, use: 
				./run_reg_test.pl -create_golden vtr_reg_nightly

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt

Create golden results with:
	<scripts_path>/parse_vtr_task.pl -create_golden -l <tasks_path>/regression_tests/vtr_reg_nightly/task_list.txt


/**************
 * LEVEL FOUR *
 *************/

/* Weekly VTR Regression - "vtr_reg_weekly"
 * To be run by automated build system every weekend.

Architecture/Benchmark:
	- vtr_reg_fpu_hard_block_arch
	- vtr_reg_qor

Estimated Runtime: 20+ hours

DO-IT-ALL COMMAND - This command will execute, parse, and check results.
				./run_reg_test.pl vtr_reg_weekly
To create golden results, use: 
				./run_reg_test.pl -create_golden vtr_reg_weekly

Execute with:
	<scripts_path>/run_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt

Parse results with:
	<scripts_path>/parse_vtr_task.pl -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt

Check results with:
	<scripts_path>/parse_vtr_task.pl -check_golden -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt

Create golden results with:
	<scripts_path>/parse_vtr_task.pl -create_golden -l <tasks_path>/regression_tests/vtr_reg_weekly/task_list.txt


/******************************
 * COMMUNICATING WITH IRC BOT *
 *****************************/

/* The IRC bot is on the following channels: #vpr or #vtr
 * Use http://chat.efnet.org:9090/ to login.
 * Provide your preferred nickname and use one of the channels 
 * mentioned above: #vpr or #vtr

/* The IRC bot responds to the following commands:

'vtr-bot: list builders'
	- Emit a list of all configured builders 

'vtr-bot: status BUILDER'
	- Announce the status of a specific Builder: what it is doing right now. 

'vtr-bot: status all'
	- Announce the status of all Builders 

'vtr-bot: watch BUILDER'
	- If the given Builder is currently running, wait until the Build is finished and then announce the results. 

'vtr-bot: last BUILDER'
	- Return the results of the last build to run on the given Builder. 

