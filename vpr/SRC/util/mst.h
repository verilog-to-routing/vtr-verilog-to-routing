typedef struct s_mst_edge
{
    unsigned short int from_node;
    unsigned short int to_node;

}
t_mst_edge;

t_mst_edge *get_mst_of_net(int inet);
