#pragma once

#include "arch_types.h"
#include "read_circuit.h"
#include "vpr_types.h"
#include "constant_nets.h"
#include "ap_flow_enums.h"
#include "argparse_value.hpp"
#include "argparse.hpp"

struct t_options {
    /* File names */
    argparse::ArgValue<std::string> ArchFile;
    argparse::ArgValue<std::string> CircuitName;
    argparse::ArgValue<std::string> NetFile;
    argparse::ArgValue<std::string> FlatPlaceFile;
    argparse::ArgValue<std::string> PlaceFile;
    argparse::ArgValue<std::string> RouteFile;
    argparse::ArgValue<std::string> CircuitFile;
    argparse::ArgValue<std::string> ActFile;
    argparse::ArgValue<std::string> PowerFile;
    argparse::ArgValue<std::string> CmosTechFile;
    argparse::ArgValue<std::string> SDCFile;

    argparse::ArgValue<e_arch_format> arch_format;
    argparse::ArgValue<e_circuit_format> circuit_format;

    argparse::ArgValue<std::string> out_file_prefix;
    argparse::ArgValue<std::string> constraints_file;
    argparse::ArgValue<std::string> write_rr_graph_file;
    argparse::ArgValue<std::string> read_rr_graph_file;
    argparse::ArgValue<std::string> read_rr_edge_override_file;
    argparse::ArgValue<std::string> write_initial_place_file;
    argparse::ArgValue<std::string> read_initial_place_file;
    argparse::ArgValue<std::string> read_vpr_constraints_file;
    argparse::ArgValue<std::string> write_vpr_constraints_file;
    argparse::ArgValue<std::string> write_constraints_file;
    argparse::ArgValue<std::string> read_flat_place_file;
    argparse::ArgValue<std::string> write_flat_place_file;

    argparse::ArgValue<std::string> write_placement_delay_lookup;
    argparse::ArgValue<std::string> read_placement_delay_lookup;

    argparse::ArgValue<std::string> write_router_lookahead;
    argparse::ArgValue<std::string> read_router_lookahead;

    argparse::ArgValue<std::string> write_intra_cluster_router_lookahead;
    argparse::ArgValue<std::string> read_intra_cluster_router_lookahead;

    argparse::ArgValue<std::string> write_block_usage;

    /* Stage Options */
    argparse::ArgValue<bool> do_packing;
    argparse::ArgValue<bool> do_legalize;
    argparse::ArgValue<bool> do_placement;
    argparse::ArgValue<bool> do_analytical_placement;
    argparse::ArgValue<bool> do_routing;
    argparse::ArgValue<bool> do_analysis;
    argparse::ArgValue<bool> do_power;

    /* Graphics Options */
    argparse::ArgValue<bool> show_graphics; ///<Enable argparse::ArgValue<int>eractive graphics?
    argparse::ArgValue<int> GraphPause;
    argparse::ArgValue<bool> save_graphics;
    argparse::ArgValue<std::string> graphics_commands;

    /* General options */
    argparse::ArgValue<bool> show_help;
    argparse::ArgValue<bool> show_version;
    argparse::ArgValue<bool> show_arch_resources;
    argparse::ArgValue<size_t> num_workers;
    argparse::ArgValue<bool> timing_analysis;
    argparse::ArgValue<e_timing_update_type> timing_update_type;
    argparse::ArgValue<bool> CreateEchoFile;
    argparse::ArgValue<bool> verify_file_digests;
    argparse::ArgValue<bool> verify_route_file_switch_id;
    argparse::ArgValue<std::string> device_layout;
    argparse::ArgValue<float> target_device_utilization;
    argparse::ArgValue<e_constant_net_method> constant_net_method;
    argparse::ArgValue<e_clock_modeling> clock_modeling;
    argparse::ArgValue<bool> two_stage_clock_routing;
    argparse::ArgValue<bool> exit_before_pack;
    argparse::ArgValue<bool> strict_checks;
    argparse::ArgValue<std::string> disable_errors;
    argparse::ArgValue<std::string> suppress_warnings;
    argparse::ArgValue<bool> allow_dangling_combinational_nodes;
    argparse::ArgValue<bool> terminate_if_timing_fails;

    /* Server options */
    argparse::ArgValue<bool> is_server_mode_enabled;
    argparse::ArgValue<int> server_port_num;

    /* Atom netlist options */
    argparse::ArgValue<bool> absorb_buffer_luts;
    argparse::ArgValue<e_const_gen_inference> const_gen_inference;
    argparse::ArgValue<bool> sweep_dangling_primary_ios;
    argparse::ArgValue<bool> sweep_dangling_nets;
    argparse::ArgValue<bool> sweep_dangling_blocks;
    argparse::ArgValue<bool> sweep_constant_primary_outputs;
    argparse::ArgValue<int> netlist_verbosity;

    /* Analytical Placement options */
    argparse::ArgValue<e_ap_analytical_solver> ap_analytical_solver;
    argparse::ArgValue<e_ap_partial_legalizer> ap_partial_legalizer;
    argparse::ArgValue<e_ap_full_legalizer> ap_full_legalizer;
    argparse::ArgValue<e_ap_detailed_placer> ap_detailed_placer;
    argparse::ArgValue<std::vector<std::string>> ap_partial_legalizer_target_density;
    argparse::ArgValue<std::vector<std::string>> appack_max_dist_th;
    argparse::ArgValue<std::vector<std::string>> appack_unrelated_clustering_args;
    argparse::ArgValue<int> ap_verbosity;
    argparse::ArgValue<float> ap_timing_tradeoff;
    argparse::ArgValue<int> ap_high_fanout_threshold;
    argparse::ArgValue<bool> ap_generate_mass_report;

    /* Clustering options */
    argparse::ArgValue<bool> connection_driven_clustering;
    argparse::ArgValue<e_unrelated_clustering> allow_unrelated_clustering;
    argparse::ArgValue<float> timing_gain_weight;
    argparse::ArgValue<float> connection_gain_weight;
    argparse::ArgValue<bool> timing_driven_clustering;
    argparse::ArgValue<e_cluster_seed> cluster_seed_type;
    argparse::ArgValue<bool> enable_clustering_pin_feasibility_filter;
    argparse::ArgValue<e_balance_block_type_util> balance_block_type_utilization;
    argparse::ArgValue<std::vector<std::string>> target_external_pin_util;
    argparse::ArgValue<bool> pack_prioritize_transitive_connectivity;
    argparse::ArgValue<int> pack_transitive_fanout_threshold;
    argparse::ArgValue<int> pack_feasible_block_array_size;
    argparse::ArgValue<std::vector<std::string>> pack_high_fanout_threshold;
    argparse::ArgValue<int> pack_verbosity;
    /* Placement options */
    argparse::ArgValue<int> Seed;
    argparse::ArgValue<bool> ShowPlaceTiming;
    argparse::ArgValue<float> PlaceInnerNum;
    argparse::ArgValue<float> place_auto_init_t_scale;
    argparse::ArgValue<float> PlaceInitT;
    argparse::ArgValue<float> PlaceExitT;
    argparse::ArgValue<float> PlaceAlphaT;
    argparse::ArgValue<e_sched_type> anneal_sched_type;
    argparse::ArgValue<e_place_algorithm> PlaceAlgorithm;
    argparse::ArgValue<e_place_algorithm> PlaceQuenchAlgorithm;
    argparse::ArgValue<e_pad_loc_type> pad_loc_type;
    argparse::ArgValue<int> PlaceChanWidth;
    argparse::ArgValue<float> place_rlim_escape_fraction;
    argparse::ArgValue<std::string> place_move_stats_file;
    argparse::ArgValue<int> placement_saves_per_temperature;
    argparse::ArgValue<e_place_effort_scaling> place_effort_scaling;
    argparse::ArgValue<e_place_delta_delay_algorithm> place_delta_delay_matrix_calculation_method;
    argparse::ArgValue<std::vector<float>> place_static_move_prob;
    argparse::ArgValue<int> place_high_fanout_net;
    argparse::ArgValue<e_place_bounding_box_mode> place_bounding_box_mode;

    argparse::ArgValue<bool> RL_agent_placement;
    argparse::ArgValue<bool> place_agent_multistate;
    argparse::ArgValue<bool> place_checkpointing;
    argparse::ArgValue<float> place_agent_epsilon;
    argparse::ArgValue<float> place_agent_gamma;
    argparse::ArgValue<float> place_dm_rlim;
    argparse::ArgValue<e_agent_space> place_agent_space;
    argparse::ArgValue<e_agent_algorithm> place_agent_algorithm;
    argparse::ArgValue<std::string> place_reward_fun;
    argparse::ArgValue<float> place_crit_limit;
    argparse::ArgValue<int> place_constraint_expand;
    argparse::ArgValue<bool> place_constraint_subtile;
    argparse::ArgValue<int> floorplan_num_horizontal_partitions;
    argparse::ArgValue<int> floorplan_num_vertical_partitions;
    argparse::ArgValue<bool> place_quench_only;

    argparse::ArgValue<int> placer_debug_block;
    argparse::ArgValue<int> placer_debug_net;

    /*NoC Options*/
    argparse::ArgValue<bool> noc;
    argparse::ArgValue<std::string> noc_flows_file;
    argparse::ArgValue<std::string> noc_routing_algorithm;
    argparse::ArgValue<double> noc_placement_weighting;
    argparse::ArgValue<double> noc_agg_bandwidth_weighting;
    argparse::ArgValue<double> noc_latency_constraints_weighting;
    argparse::ArgValue<double> noc_latency_weighting;
    argparse::ArgValue<double> noc_congestion_weighting;
    argparse::ArgValue<double> noc_centroid_weight;
    argparse::ArgValue<double> noc_swap_percentage;
    argparse::ArgValue<int> noc_sat_routing_bandwidth_resolution;
    argparse::ArgValue<int> noc_sat_routing_latency_overrun_weighting_factor;
    argparse::ArgValue<int> noc_sat_routing_congestion_weighting_factor;
    argparse::ArgValue<int> noc_sat_routing_num_workers;
    argparse::ArgValue<bool> noc_sat_routing_log_search_progress;
    argparse::ArgValue<std::string> noc_placement_file_name;

    /* Timing-driven placement options only */
    argparse::ArgValue<float> PlaceTimingTradeoff;
    argparse::ArgValue<int> RecomputeCritIter;
    argparse::ArgValue<int> inner_loop_recompute_divider;
    argparse::ArgValue<int> quench_recompute_divider;
    argparse::ArgValue<float> place_exp_first;
    argparse::ArgValue<float> place_exp_last;
    argparse::ArgValue<float> place_delay_offset;
    argparse::ArgValue<int> place_delay_ramp_delta_threshold;
    argparse::ArgValue<float> place_delay_ramp_slope;
    argparse::ArgValue<float> place_tsu_rel_margin;
    argparse::ArgValue<float> place_tsu_abs_margin;
    argparse::ArgValue<std::string> post_place_timing_report_file;
    argparse::ArgValue<PlaceDelayModelType> place_delay_model;
    argparse::ArgValue<e_reducer> place_delay_model_reducer;
    argparse::ArgValue<std::string> allowed_tiles_for_delay_model;

    /* Router Options */
    argparse::ArgValue<bool> check_rr_graph;
    argparse::ArgValue<int> max_router_iterations;
    argparse::ArgValue<float> first_iter_pres_fac;
    argparse::ArgValue<float> initial_pres_fac;
    argparse::ArgValue<float> pres_fac_mult;
    argparse::ArgValue<float> max_pres_fac;
    argparse::ArgValue<float> acc_fac;
    argparse::ArgValue<int> bb_factor;
    argparse::ArgValue<e_base_cost_type> base_cost_type;
    argparse::ArgValue<float> bend_cost;
    argparse::ArgValue<e_route_type> RouteType;
    argparse::ArgValue<int> RouteChanWidth;
    argparse::ArgValue<int> min_route_chan_width_hint; ///<Hint to binary search router about what the min chan width is
    argparse::ArgValue<bool> verify_binary_search;
    argparse::ArgValue<e_router_algorithm> RouterAlgorithm;
    argparse::ArgValue<int> min_incremental_reroute_fanout;
    argparse::ArgValue<bool> read_rr_edge_metadata;
    argparse::ArgValue<bool> exit_after_first_routing_iteration;
    argparse::ArgValue<e_check_route_option> check_route;
    argparse::ArgValue<size_t> max_logged_overused_rr_nodes;
    argparse::ArgValue<bool> generate_rr_node_overuse_report;
    argparse::ArgValue<e_rr_node_reorder_algorithm> reorder_rr_graph_nodes_algorithm;
    argparse::ArgValue<int> reorder_rr_graph_nodes_threshold;
    argparse::ArgValue<int> reorder_rr_graph_nodes_seed;
    argparse::ArgValue<bool> flat_routing;
    argparse::ArgValue<bool> router_opt_choke_points;
    argparse::ArgValue<int> route_verbosity;
    argparse::ArgValue<int> custom_3d_sb_fanin_fanout;

    /* Timing-driven router options only */
    argparse::ArgValue<float> astar_fac;
    argparse::ArgValue<float> astar_offset;
    argparse::ArgValue<float> router_profiler_astar_fac;
    argparse::ArgValue<bool> enable_parallel_connection_router;
    argparse::ArgValue<float> post_target_prune_fac;
    argparse::ArgValue<float> post_target_prune_offset;
    argparse::ArgValue<int> multi_queue_num_threads;
    argparse::ArgValue<int> multi_queue_num_queues;
    argparse::ArgValue<bool> multi_queue_direct_draining;
    argparse::ArgValue<float> max_criticality;
    argparse::ArgValue<float> criticality_exp;
    argparse::ArgValue<float> router_init_wirelength_abort_threshold;
    argparse::ArgValue<e_incr_reroute_delay_ripup> incr_reroute_delay_ripup;
    argparse::ArgValue<e_routing_failure_predictor> routing_failure_predictor;
    argparse::ArgValue<e_routing_budgets_algorithm> routing_budgets_algorithm;
    argparse::ArgValue<bool> save_routing_per_iteration;
    argparse::ArgValue<float> congested_routing_iteration_threshold_frac;
    argparse::ArgValue<e_route_bb_update> route_bb_update;
    argparse::ArgValue<int> router_high_fanout_threshold;
    argparse::ArgValue<float> router_high_fanout_max_slope;
    argparse::ArgValue<int> router_debug_net;
    argparse::ArgValue<int> router_debug_sink_rr;
    argparse::ArgValue<int> router_debug_iteration;
    argparse::ArgValue<e_router_lookahead> router_lookahead_type;
    argparse::ArgValue<bool> generate_router_lookahead_report;
    argparse::ArgValue<double> router_initial_acc_cost_chan_congestion_threshold;
    argparse::ArgValue<double> router_initial_acc_cost_chan_congestion_weight;
    argparse::ArgValue<int> router_max_convergence_count;
    argparse::ArgValue<float> router_reconvergence_cpd_threshold;
    argparse::ArgValue<bool> router_update_lower_bound_delays;
    argparse::ArgValue<std::string> router_first_iteration_timing_report_file;
    argparse::ArgValue<e_router_initial_timing> router_initial_timing;
    argparse::ArgValue<e_heap_type> router_heap;

    /* Analysis options */
    argparse::ArgValue<bool> full_stats;
    argparse::ArgValue<bool> Generate_Post_Synthesis_Netlist;
    argparse::ArgValue<bool> Generate_Post_Implementation_Merged_Netlist;
    argparse::ArgValue<bool> generate_post_implementation_sdc;
    argparse::ArgValue<int> timing_report_npaths;
    argparse::ArgValue<e_timing_report_detail> timing_report_detail;
    argparse::ArgValue<bool> timing_report_skew;
    argparse::ArgValue<std::string> echo_dot_timing_graph_node;
    argparse::ArgValue<e_post_synth_netlist_unconn_handling> post_synth_netlist_unconn_input_handling;
    argparse::ArgValue<e_post_synth_netlist_unconn_handling> post_synth_netlist_unconn_output_handling;
    argparse::ArgValue<bool> post_synth_netlist_module_parameters;
    argparse::ArgValue<std::string> write_timing_summary;
    argparse::ArgValue<bool> skip_sync_clustering_and_routing_results;
    argparse::ArgValue<bool> generate_net_timing_report;
};

argparse::ArgumentParser create_arg_parser(const std::string& prog_name, t_options& args);
t_options read_options(int argc, const char** argv);
void set_conditional_defaults(t_options& args);
bool verify_args(const t_options& args);
