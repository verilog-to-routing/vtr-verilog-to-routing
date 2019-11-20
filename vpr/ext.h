/* Graphics stuff */
extern int showgraph;  /* 1 for X graphics, 0 for no graphics */
extern int show_nets;  /* 1 if you want nets drawn, 0 otherwise. */
extern int automode;   /* 0 for always pause, 1 sometimes, 2 never */
extern enum pic_type pic_on_screen;

/* Annealing schedule stuff */ 
extern int sched_type, inner_num; 
extern float init_t, alpha_t, exit_t; 

/* Netlist to be placed stuff. */
extern num_nets, num_blocks;     
extern num_p_inputs, num_p_outputs, num_clbs, num_globals;
extern struct s_net *net;
extern struct s_block *block;
extern int *is_global;
extern int **net_pin_class;

/* Physical FPGA architecture stuff */
extern int nx, ny, io_rat, clb_size;
extern float chan_width_io;
extern struct s_chan chan_x_dist, chan_y_dist;   /* functional form */
extern int **pinloc;
extern int *clb_pin_class;
extern struct s_class *class_inf;
extern int num_class;
/* chan_width_x is the x-directed channel; i.e. between rows */
extern int *chan_width_x, *chan_width_y; /* numerical form */
extern struct s_clb **clb;
extern struct s_bb *bb_coords, *bb_num_on_edges;
extern float **chanx_place_cost_fac, **chany_place_cost_fac;
extern const float cross_count[]; 
   /* expected crossing count of depends on net size */

/* Structures used by router and graphics. */
extern struct s_phys_chan **chan_x, **chan_y;
   /* chan_x [1..nx][0..ny]  chan_y [0..nx][1..ny] */
extern struct s_trace **trace_head, **trace_tail;



/* PROCEDURES */
 /* Updates screen if proper flags and priority.    */
extern void update_screen (int priority, char *msg); 
