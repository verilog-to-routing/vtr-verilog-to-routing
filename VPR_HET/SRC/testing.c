#include "util.h"
#include "vpr_types.h"
#include "globals_declare.h"
#include "read_arch.h"
#include "rr_graph.h"
#include "draw.h"
#include "graphics.h"
#include <assert.h>

int
main()
{
    char msg[BUFSIZE] = "This is a test.";
    struct s_det_routing_arch det_routing_arch;
    t_segment_inf *segment_inf;
    t_timing_inf timing_inf;
    t_subblock_data subblock_data;
    t_chan_width_dist chan_width_dist;
    int nodes_per_chan;
    int i, j;

    read_arch("test.arch", DETAILED, &det_routing_arch, &segment_inf,
	      &timing_inf, &subblock_data, &chan_width_dist);

    print_arch("test.arch", DETAILED, det_routing_arch, segment_inf,
	       timing_inf, subblock_data, chan_width_dist);

    num_clbs = 64;
    init_arch(1., FALSE);
    printf("nx = %d; ny = %d\n", nx, ny);

    printf("Setting Nodes Per Channel ...\n");
    /* nodes_per_chan = 32; */
    nodes_per_chan = 8;
    for(i = 0; i < nx + 1; i++)
	chan_width_x[i] = nodes_per_chan;
    for(i = 0; i < ny + 1; i++)
	chan_width_y[i] = nodes_per_chan;

    printf("Building rr_graph ...\n");
    build_rr_graph(DETAILED, det_routing_arch, segment_inf, timing_inf,
		   INTRINSIC_DELAY);
    printf("Dumpping rr_graph ...\n");
    dump_rr_graph("rr_graph.echo");

    printf("Done.\n");

    printf("num_nets = %d\n", num_nets);
    printf("s_net = %d\n", (int)net);

    num_nets = 0;
    num_blocks = 0;
    clb = my_malloc(sizeof(struct s_clb *) * (nx + 2));
    for(i = 0; i < nx + 2; i++)
	{
	    clb[i] = my_malloc(sizeof(struct s_clb) * (ny + 2));
	}
    for(i = 0; i < nx + 2; i++)
	{
	    for(j = 0; j < ny + 2; j++)
		{
		    clb[i][j].type = CLB;
		    clb[i][j].occ = 0;
		    clb[i][j].u.block = 0;
		}
	}
    for(i = 0; i < nx + 2; i++)
	{
	    clb[i][0].type = IO;
	    clb[i][ny + 1].type = IO;
	    clb[i][0].u.io_blocks = my_malloc(sizeof(int) * io_rat);
	    clb[i][ny + 1].u.io_blocks = my_malloc(sizeof(int) * io_rat);
	}
    for(j = 0; j < ny + 2; j++)
	{
	    clb[0][j].type = IO;
	    clb[nx + 1][j].type = IO;
	    clb[0][j].u.io_blocks = my_malloc(sizeof(int) * io_rat);
	    clb[nx + 1][j].u.io_blocks = my_malloc(sizeof(int) * io_rat);
	}
    clb[0][0].type = ILLEGAL;
    clb[0][ny + 1].type = ILLEGAL;
    clb[nx + 1][0].type = ILLEGAL;
    clb[nx + 1][ny + 1].type = ILLEGAL;

    set_graphics_state(TRUE, 0, DETAILED);
    init_graphics("testing drawing capabilities");
    alloc_draw_structs();
    init_draw_coords(pins_per_clb);
    printf("num_rr_nodes = %d\n", num_rr_nodes);
    update_screen(MAJOR, msg, ROUTING, FALSE);
    while(1);
    close_graphics();

    return 0;
}
