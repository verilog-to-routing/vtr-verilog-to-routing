/* type definitions */  
    struct s_clb_grid
{
    boolean IsAuto;
    float Aspect;
     int W;
     int H;
 };
typedef struct s_arch t_arch;
struct s_arch
{
    t_chan_width_dist Chans;
    int N;			/* Cluster size */
     int K;			/* LUT size */
     enum e_switch_block_type SBType;
     float R_minW_nmos;
     float R_minW_pmos;
     int Fs;
     float C_ipin_cblock;
     float T_ipin_cblock;
     float grid_logic_tile_area;
     float ipin_mux_trans_size;
     struct s_clb_grid clb_grid;
     t_segment_inf * Segments;
     int num_segments;
     struct s_switch_inf *Switches;
     int num_switches;
 };


/* function declarations */ 
void XmlReadArch(IN const char *ArchFile,
		 IN boolean timing_enabled,
		 OUT struct s_arch *arch,
		 OUT t_type_descriptor ** Types,
		 OUT int *NumTypes);
void EchoArch(IN const char *EchoFile,
	       IN const t_type_descriptor * Types,
	       IN int NumTypes);


