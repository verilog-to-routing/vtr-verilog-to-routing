

/* // Define net statistic 
struct Odin_net_statistic{
    int depth_net;
    unsigned int max_dist_out;
    unsigned int max_dis_in;
    float avg_max_dist; 

}; 

{ (function: free_nnode)
 /*-------------------------------------------------------------------------------------------
nnode_t *free_nnode(nnode_t *to_free)
{
	if (to_free)
	{
		/* need to free node_data 
		if(to_free->karim)
			vtr::free(to_free->karim);
		
}
/*-----------------------------------------------------------------------
 * (Depth_Traverse_First), 
 *-----------------------------------------------------------------------
void depth_traverse_counter(nnet_t *net, short traverse_mark_number, int *count)
{
    int a, b; 
    nnet_t *next_net; 
    
    if (net->traverse_visited == traverse_mark_number)
    {
        return;
    
    }
else    
for (i=0 ; i < net->num_output_pins; i++)
{
    if (net->num_output_pins[i]-.net ==Null)
        continue;

    next_net = net->output_pins[i]->net; 
    for (j=0 ; j < next_net->num_fnout_pins ; j++)
    {
        if (next_net->fanout_pin[j] == NULL)
            continue;
        
        next_net = next_net ->fanoout_pins[j]->net; 
        if (next_net == NULL)
            continue;   

            depth_traverse_count(next_net, count, traverse_mark_number);

    } 

}
}
    #include <stdio.h>
    #include <stdlib.h>
     
    struct node
    {
        int vertex;
        struct node* next;
    };
    struct net* createNet(int v);
    struct Graph
    {
        int numVertices;
        int* visited;
        struct net** adjLists; // we need int** to store a two dimensional array. Similary, we need struct node** to store an array of Linked lists
    };
    struct Graph* createGraph(int);
    void addEdge(struct Graph*, int, int);
    void printGraph(struct Graph*);
    void DFS(struct Graph*, int);
    int main()
    {
        struct Graph* graph = createGraph(4);
        addEdge(graph, 0, 1);
        addEdge(graph, 0, 2);
        addEdge(graph, 1, 2);
        addEdge(graph, 2, 3);
        
        printGraph(graph);
        DFS(graph, 2);
        
        return 0;
    }
    void DFS(struct Graph* graph, int vertex) {
            struct net* adjList = graph->adjLists[vertex];
            struct net* temp = adjList;
            
            graph->visited[vertex] = 1;
            printf("Visited %d \n", vertex);
        
            while(temp!=NULL) {
                int connectedVertex = temp->vertex;
            
                if(graph->visited[connectedVertex] == 0) {
                    DFS(graph, connectedVertex);
                }
                temp = temp->next;
            }       
    }
     
    struct net* createNet(int v)
    {
        struct net* newNet = malloc(sizeof(struct net));
        newNete->vertex = v;
        newNet->next = NULL;
        return newNet;
    }
    struct Graph* createGraph(int vertices)
    {
        struct Graph* graph = malloc(sizeof(struct Graph));
        graph->numVertices = vertices;
     
        graph->adjLists = malloc(vertices * sizeof(struct net*));
        
        graph->visited = malloc(vertices * sizeof(int));
     
        int i;
        for (i = 0; i < vertices; i++) {
            graph->adjLists[i] = NULL;
            graph->visited[i] = 0;
        }
        return graph;
    }
     
    void addEdge(struct Graph* graph, int src, int dest)
    {
        // Add edge from src to dest
        struct net* newNet = createNet(dest);
        newNet->next = graph->adjLists[src];
        graph->adjLists[src] = newNet;
     
        // Add edge from dest to src
        newNet = createNet(src);
        newNet->next = graph->adjLists[dest];
        graph->adjLists[dest] = newNet;
    }
     
    void printGraph(struct Graph* graph)
    {
        int v;
        for (v = 0; v < graph->numVertices; v++)
        {
            struct net* temp = graph->adjLists[v];
            printf("\n Adjacency list of vertex %d\n ", v);
            while (temp)
            {
                printf("%d -> ", temp->vertex);
                temp = temp->next;
            }
            printf("\n");
        }
    }

*/ /*
#include <stdio.h>
int main(){

}
return 0; */
