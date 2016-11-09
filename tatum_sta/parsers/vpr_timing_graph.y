%{

#include <stdio.h>
#include <cstring>
#include <string>
#include <cmath>
#include <vector>
#include <iostream>

#include "tatum_assert.hpp"
#include "tatum_error.hpp"
#include "vpr_timing_graph_common.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

int yyerror(const tatum::TimingGraph& tg, const VprArrReqTimes& arr_req_times, const tatum::TimingConstraints& tc, const std::vector<BlockId>& node_logical_blocks, const std::vector<float>& edge_delays, const char *msg);
extern int yylex(void);
extern int yylineno;
extern char* yytext;

using std::endl;
using std::cout;

int arr_req_cnt = 0;
int from_clock_domain = 0;
int to_clock_domain = 0;

std::vector<std::vector<edge_t>*> node_out_edges;

using tatum::NodeId;
using tatum::EdgeId;
using tatum::DomainId;
using tatum::NodeType;
using tatum::TimingGraph;
using tatum::TimingConstraints;

%}


/* Verbose error reporting */
%error-verbose
%parse-param{tatum::TimingGraph& timing_graph}
%parse-param{VprArrReqTimes& arr_req_times}
%parse-param{tatum::TimingConstraints& timing_constraints}
%parse-param{std::vector<BlockId>& node_logical_blocks}
%parse-param{std::vector<float>& edge_delays}

%union {
    char* strVal;
    double floatVal;
    int intVal;
    domain_skew_iodelay_t domainSkewIodelayVal;
    edge_t edgeVal;
    node_arr_req_t nodeArrReqVal;
    timing_graph_level_t timingGraphLevelVal;
    node_t nodeVal;
    VprNodeType nodeTypeVal;
    domain_header_t domain_header;
};

/* declare constant tokens */
%token TGRAPH_HEADER          "timing_graph_header"
%token NUM_TNODES             "num_tnodes:"
%token NUM_TNODE_LEVELS       "num_tnode_levels:"
%token LEVEL                  "Level:"
%token NUM_LEVEL_NODES        "Num_nodes:"
%token NODES                  "Nodes:"
%token NET_DRIVER_TNODE_HEADER "Net #\tNet_to_driver_tnode"
%token NODE_ARR_REQ_HEADER    "node_req_arr_header"
%token SRC_DOMAIN             "SRC_Domain:"
%token SINK_DOMAIN            "SINK_Domain:"
%token CLOCK_CONSTRAINTS_HEADER "Clock Constraints"
%token INPUT_CONSTRAINTS_HEADER "Input Constraints"
%token OUTPUT_CONSTRAINTS_HEADER "Output Constraints"
%token CLOCK_CONSTRAINTS_COLS "Src_Clk\tSink_Clk\tConstraint"
%token INPUT_CONSTRAINTS_COLS "tnode_id\tinput_delay"
%token OUTPUT_CONSTRAINTS_COLS "tnode_id\toutput_delay"

%token TN_INPAD_SOURCE        "TN_INPAD_SOURCE"
%token TN_INPAD_OPIN          "TN_INPAD_OPIN"
%token TN_OUTPAD_IPIN         "TN_OUTPAD_IPIN"
%token TN_OUTPAD_SINK         "TN_OUTPAD_SINK"
%token TN_CB_IPIN             "TN_CB_IPIN"
%token TN_CB_OPIN             "TN_CB_OPIN"
%token TN_INTERMEDIATE_NODE   "TN_INTERMEDIATE_NODE"
%token TN_PRIMITIVE_IPIN      "TN_PRIMITIVE_IPIN"
%token TN_PRIMITIVE_OPIN      "TN_PRIMITIVE_OPIN"
%token TN_FF_IPIN             "TN_FF_IPIN"
%token TN_FF_OPIN             "TN_FF_OPIN"
%token TN_FF_SINK             "TN_FF_SINK"
%token TN_FF_SOURCE           "TN_FF_SOURCE"
%token TN_FF_CLOCK            "TN_FF_CLOCK"
%token TN_CLOCK_SOURCE        "TN_CLOCK_SOURCE"
%token TN_CLOCK_OPIN          "TN_CLOCK_OPIN"
%token TN_CONSTANT_GEN_SOURCE "TN_CONSTANT_GEN_SOURCE"

%token EOL "end-of-line"
%token TAB "tab-character"

/* declare variable tokens */
%token <floatVal> FLOAT_NUMBER
%token <intVal> INT_NUMBER

/* declare types */
%type <floatVal> number
%type <floatVal> float_number
%type <intVal> int_number

%type <intVal> num_tnodes
%type <intVal> node_id
%type <intVal> num_out_edges
%type <intVal> num_tnode_levels

%type <nodeTypeVal> tnode_type
%type <strVal> TN_INPAD_SOURCE
%type <strVal> TN_INPAD_OPIN
%type <strVal> TN_OUTPAD_IPIN
%type <strVal> TN_OUTPAD_SINK
%type <strVal> TN_CB_IPIN
%type <strVal> TN_CB_OPIN
%type <strVal> TN_INTERMEDIATE_NODE
%type <strVal> TN_PRIMITIVE_IPIN
%type <strVal> TN_PRIMITIVE_OPIN
%type <strVal> TN_FF_IPIN
%type <strVal> TN_FF_OPIN
%type <strVal> TN_FF_SINK
%type <strVal> TN_FF_SOURCE
%type <strVal> TN_FF_CLOCK
%type <strVal> TN_CLOCK_SOURCE
%type <strVal> TN_CLOCK_OPIN
%type <strVal> TN_CONSTANT_GEN_SOURCE

%type <edgeVal> tedge
%type <nodeArrReqVal> node_arr_req_time
%type <floatVal> t_arr_req
%type <timingGraphLevelVal> timing_graph_level
%type <nodeVal> tnode
%type <intVal> src_domain
%type <intVal> sink_domain
%type <domain_header> domain_header
%type <floatVal> constraint

%type <intVal> is_clk_src
%type <floatVal> io_delay
%type <floatVal> skew
%type <intVal> domain
%type <intVal> ipin
%type <intVal> iblk


/* Top level rule */
%start finish
%%

finish: timing_graph timing_constraints EOL {
                                                /*
                                                 *timing_graph.add_launch_capture_edges();
                                                 *timing_graph.fill_back_edges();
                                                 *timing_graph.levelize();
                                                 */
                                                //Add the edges, now that all nodes have been added
                                                for(int src_node_id = 0; src_node_id < (int) node_out_edges.size(); src_node_id++) {
                                                    for(const auto& edge : *node_out_edges[src_node_id]) {
                                                        EdgeId edge_id = timing_graph.add_edge(NodeId(edge.src_node), NodeId(edge.sink_node));

                                                        //Record the edge delay
                                                        if(size_t(edge_id) >= edge_delays.size()) {
                                                            edge_delays.resize(size_t(edge_id)+1);
                                                        }
                                                        edge_delays[size_t(edge_id)] = edge.delay;
                                                    }
                                                    //Clean-up
                                                    delete node_out_edges[src_node_id];
                                                }
                                            }

timing_graph: num_tnodes                    { printf("Loading Timing Graph with %d nodes\n", $1); arr_req_times.set_num_nodes($1); }
    | timing_graph TGRAPH_HEADER EOL        { /*printf("Timing Graph file Header\n");*/ }
    | timing_graph tnode                    {

                                                /*
                                                 *cout << "Node " << $2.node_id << ", ";
                                                 *cout << "Type " << $2.type << ", ";
                                                 *cout << "ipin " << $2.ipin << ", ";
                                                 *cout << "iblk " << $2.iblk << ", ";
                                                 *cout << "domain " << $2.domain << ", ";
                                                 *cout << "is_clk_src " << $2.is_clk_src << ", ";
                                                 *cout << "skew " << $2.skew << ", ";
                                                 *cout << "iodelay " << $2.iodelay << ", ";
                                                 *cout << "edges " << $2.out_edges->size();
                                                 *cout << endl;
                                                 */

                                                //Add the node
                                                DomainId domain_id;
                                                if($2.domain >= 0) {
                                                    domain_id = timing_constraints.create_clock_domain(std::to_string($2.domain));
                                                }
                                                NodeId src_node_id = timing_graph.add_node($2.type, domain_id, $2.is_clk_src);

                                                if($2.is_clk_src) {
                                                    cout << "Marking clock source for " << domain_id <<  " " << src_node_id << endl;
                                                    timing_constraints.set_clock_domain_source(src_node_id, domain_id);
                                                }
                                                if($2.is_const_gen) {
                                                    cout << "Marking const gen for " << src_node_id << endl;
                                                    timing_constraints.set_constant_generator(src_node_id);
                                                }

                                                //Save the edges to be added later
                                                node_out_edges.push_back($2.out_edges);
                                                node_logical_blocks.emplace_back($2.iblk);

                                                TATUM_ASSERT(size_t(src_node_id) == node_out_edges.size() - 1);

                                                if(timing_graph.nodes().size() % 1000000 == 0) {
                                                    std::cout << "Loaded " << timing_graph.nodes().size() / 1e6 << "M nodes..." << std::endl;
                                                }
                                                TATUM_ASSERT(timing_graph.nodes().size() - 1 == size_t($2.node_id));

                                            }
    | timing_graph num_tnode_levels         {
                                                /*timing_graph.set_num_levels($2);*/
                                                /*printf("Timing Graph Levels %d\n", $2);*/
                                            }
    | timing_graph timing_graph_level EOL   {
                                                /*timing_graph.add_level($2.level, $2.node_ids);*/
                                                /*printf("TG level %d, Nodes %zu\n", $2.level, $2.node_ids.size()); */
                                            }
    | timing_graph NET_DRIVER_TNODE_HEADER  {/*printf("Net to driver Tnode header\n");*/}
    | timing_graph NODE_ARR_REQ_HEADER EOL  {
                                                printf("Loading Arr & Req Times\n");
                                            }
    | timing_graph domain_header EOL        {
                                                printf("Clock Domain SRC->SINK: %d -> %d\n", $2.src_domain, $2.sink_domain);
                                                from_clock_domain = $2.src_domain;
                                                to_clock_domain = $2.sink_domain;

                                            }
    | timing_graph node_arr_req_time        {
                                                TATUM_ASSERT(from_clock_domain >= 0);
                                                TATUM_ASSERT(to_clock_domain >= 0);
                                                arr_req_times.add_arr_time(timing_constraints.create_clock_domain(std::to_string(from_clock_domain)), NodeId($2.node_id), $2.T_arr);
                                                arr_req_times.add_req_time(timing_constraints.create_clock_domain(std::to_string(from_clock_domain)), NodeId($2.node_id), $2.T_req);

                                                arr_req_cnt++;
                                                if(arr_req_cnt % 1000000 == 0) {
                                                    std::cout << "Loaded " << arr_req_cnt / 1e6 << "M arr/req times..." << std::endl;
                                                }
                                            }
    | timing_graph EOL {}

tnode: node_id tnode_type ipin iblk domain is_clk_src skew io_delay num_out_edges {
                                                                      $$.node_id = $1;
                                                                      switch($2) {
                                                                        case VprNodeType::INPAD_SOURCE:
                                                                            $$.type = tatum::NodeType::INPAD_SOURCE;
                                                                            break;
                                                                        case VprNodeType::FF_SOURCE:
                                                                            $$.type = tatum::NodeType::FF_SOURCE;
                                                                            break;
                                                                        case VprNodeType::CLOCK_SOURCE:
                                                                            $$.type = tatum::NodeType::CLOCK_SOURCE;
                                                                            break;
                                                                        case VprNodeType::CONSTANT_GEN_SOURCE:
                                                                            $$.type = tatum::NodeType::CONSTANT_GEN_SOURCE;
                                                                            break;
                                                                        case VprNodeType::INPAD_OPIN:
                                                                            $$.type = tatum::NodeType::INPAD_OPIN;
                                                                            break;
                                                                        case VprNodeType::PRIMITIVE_OPIN:
                                                                            $$.type = tatum::NodeType::PRIMITIVE_OPIN;
                                                                            break;
                                                                        case VprNodeType::FF_OPIN:
                                                                            $$.type = tatum::NodeType::FF_OPIN;
                                                                            break;
                                                                        case VprNodeType::CLOCK_OPIN:
                                                                            $$.type = tatum::NodeType::CLOCK_OPIN;
                                                                            break;
                                                                        case VprNodeType::OUTPAD_IPIN:
                                                                            $$.type = tatum::NodeType::OUTPAD_IPIN;
                                                                            break;
                                                                        case VprNodeType::PRIMITIVE_IPIN:
                                                                            $$.type = tatum::NodeType::PRIMITIVE_IPIN;
                                                                            break;
                                                                        case VprNodeType::FF_IPIN:
                                                                            $$.type = tatum::NodeType::FF_IPIN;
                                                                            break;
                                                                        case VprNodeType::OUTPAD_SINK:
                                                                            $$.type = tatum::NodeType::OUTPAD_SINK;
                                                                            break;
                                                                        case VprNodeType::FF_SINK:
                                                                            $$.type = tatum::NodeType::FF_SINK;
                                                                            break;
                                                                        case VprNodeType::FF_CLOCK:
                                                                            $$.type = tatum::NodeType::FF_CLOCK;
                                                                            break;
                                                                        default:
                                                                            throw tatum::Error("Unrecognzied VPR node type");
                                                                      }
                                                                      $$.ipin = $3;
                                                                      $$.iblk = $4;
                                                                      $$.domain = $5;
                                                                      $$.is_clk_src = $6;
                                                                      $$.is_const_gen = ($2 == VprNodeType::CONSTANT_GEN_SOURCE);
                                                                      $$.skew = $7;
                                                                      $$.iodelay = $8;
                                                                      $$.out_edges = new std::vector<edge_t>;
                                                                      $$.out_edges->reserve($9);
                                                                    }
    | tnode tedge {
                    //Edges may be broken by VPR to remove
                    //combinational loops and marked with an invalid 'sink_node'
                    //We skip these edges
                    if($2.sink_node != -1) {
                        $2.src_node = $1.node_id;
                        $$.out_edges->push_back($2);
                    }
                  }

node_id: int_number TAB {$$ = $1;}

num_out_edges: int_number {$$ = $1;}

tedge: TAB int_number TAB number EOL { $$.sink_node = $2; $$.delay = $4; }
    | TAB TAB TAB TAB TAB TAB TAB TAB TAB TAB TAB int_number TAB number EOL { $$.sink_node = $12; $$.delay = $14; }

tnode_type: TN_INPAD_SOURCE TAB     { $$ = VprNodeType::INPAD_SOURCE; }
    | TN_INPAD_OPIN TAB             { $$ = VprNodeType::INPAD_OPIN; }
    | TN_OUTPAD_IPIN TAB            { $$ = VprNodeType::OUTPAD_IPIN; }
    | TN_OUTPAD_SINK TAB            { $$ = VprNodeType::OUTPAD_SINK; }
    | TN_CB_IPIN TAB                { $$ = VprNodeType::UNKOWN; }
    | TN_CB_OPIN TAB                { $$ = VprNodeType::UNKOWN; }
    | TN_INTERMEDIATE_NODE TAB      { $$ = VprNodeType::UNKOWN; }
    | TN_PRIMITIVE_IPIN TAB         { $$ = VprNodeType::PRIMITIVE_IPIN; }
    | TN_PRIMITIVE_OPIN TAB         { $$ = VprNodeType::PRIMITIVE_OPIN; }
    | TN_FF_IPIN TAB                { $$ = VprNodeType::FF_IPIN; }
    | TN_FF_OPIN TAB                { $$ = VprNodeType::FF_OPIN; }
    | TN_FF_SINK TAB                { $$ = VprNodeType::FF_SINK; }
    | TN_FF_SOURCE TAB              { $$ = VprNodeType::FF_SOURCE; }
    | TN_FF_CLOCK TAB               { $$ = VprNodeType::FF_CLOCK; }
    | TN_CLOCK_SOURCE TAB           { $$ = VprNodeType::CLOCK_SOURCE; }
    | TN_CLOCK_OPIN TAB             { $$ = VprNodeType::CLOCK_OPIN; }
    | TN_CONSTANT_GEN_SOURCE TAB    { $$ = VprNodeType::CONSTANT_GEN_SOURCE; }

num_tnodes: NUM_TNODES int_number {$$ = $2; }

/*
 * Timing Graph Levels
 */

num_tnode_levels: NUM_TNODE_LEVELS int_number {$$ = $2; }

timing_graph_level: LEVEL int_number NUM_LEVEL_NODES int_number EOL { /* $$.level = $2; $$.node_ids->reserve($4); */}
    | timing_graph_level NODES {}
    | timing_graph_level TAB int_number   {
                                            /*
                                             *$$.node_ids->push_back($3);
                                             */
                                          }


/*
 * Arrival/Required time
 */
domain_header: SRC_DOMAIN int_number TAB SINK_DOMAIN int_number {$$.src_domain = $2; $$.sink_domain = $5;}

node_arr_req_time: int_number t_arr_req t_arr_req EOL {$$.node_id = $1; $$.T_arr = $2; $$.T_req = $3;}

t_arr_req: TAB number { $$ = $2; }
    | TAB TAB '-' { $$ = NAN; }

/*
 * Node de-tabbed column values
 */

io_delay: TAB {$$ = NAN; }
    | number TAB { $$ = $1; }

skew: TAB {$$ = NAN; }
    | number TAB { $$ = $1; }

is_clk_src: TAB {$$ = 0;}
    | int_number TAB {$$ = $1;}

domain: TAB {$$ = -1; }
    | int_number TAB { $$ = $1; }

iblk: TAB { $$ = -1; }
    | int_number TAB { $$ = $1; }

ipin: TAB { $$ = -1; }
    | int_number TAB { $$ = $1; }
/*
 * Timing Constraints
 */
timing_constraints: clock_constraints input_constraints output_constraints {}

/*
 * Clock Constraints
 */
clock_constraints: CLOCK_CONSTRAINTS_HEADER EOL {}
    | clock_constraints CLOCK_CONSTRAINTS_COLS EOL {}
    | clock_constraints clock_constraint {}

clock_constraint: src_domain sink_domain constraint EOL {
                    if($3 != -1.0) {
                        //-1 is used to signal invalid constraints that should
                        //not be analyzed
                        if($1 >= 0 && $2 >= 0) { //Valid clocks
                            DomainId src_domain = timing_constraints.create_clock_domain(std::to_string($1));
                            DomainId sink_domain = timing_constraints.create_clock_domain(std::to_string($2));
                            timing_constraints.set_setup_constraint(src_domain, sink_domain, $3);

                            //FIXME: Fixing the hold constraint to zero.  This will ignore any
                            //       hold multi-cycles
                            timing_constraints.set_hold_constraint(src_domain, sink_domain, 0.);
                        }
                    }
                }

src_domain: int_number TAB { $$ = $1; }
sink_domain: int_number TAB { $$ = $1; }
constraint: number { $$ = $1; }

/*
 * Input Constraints
 */
input_constraints: INPUT_CONSTRAINTS_HEADER EOL {}
    | input_constraints INPUT_CONSTRAINTS_COLS EOL {}
    | input_constraints input_constraint {}
input_constraint: node_id constraint EOL { timing_constraints.set_input_constraint(NodeId($1), timing_graph.node_clock_domain(NodeId($1)), $2); }

/*
 * Output Constraints
 */
output_constraints: OUTPUT_CONSTRAINTS_HEADER EOL {}
    | output_constraints OUTPUT_CONSTRAINTS_COLS EOL {}
    | output_constraints output_constraint {}
output_constraint: node_id constraint EOL { timing_constraints.set_output_constraint(NodeId($1), timing_graph.node_clock_domain(NodeId($1)), $2); }

/*
 * Basic values
 */
number: float_number { $$ = $1; }
    | int_number { $$ = $1; }

float_number: FLOAT_NUMBER { $$ = $1; }

int_number: INT_NUMBER { $$ = $1; }

%%


int yyerror(const TimingGraph& /*tg*/, const VprArrReqTimes& /*arr_req_times*/, const TimingConstraints& /*tc*/, const std::vector<BlockId>& /*node_logical_blocks*/, const std::vector<float>& /*edge_delays*/, const char *msg) {
    printf("Line: %d, Text: '%s', Error: %s\n",yylineno, yytext, msg);
    return 1;
}
