/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "types.h"
#include "util.h"
#include "netlist_utils.h"
#include "print_netlist.h"


/* this function prints out the netlist 
   in the terminal, for debugging purpose */

void function_to_print_node_and_its_pin(npin_t *);
void print_netlist_for_checking (netlist_t *netlist, char *name)
{
  int i,j,k;
  npin_t * pin;
  nnet_t * net;
  nnode_t * node;
 

  printf("printing the netlist : %s\n",name);
  /* gnd_node */
  node=netlist->gnd_node;
  net=netlist->zero_net;
  printf("--------gnd_node-------\n");
  printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  for(i=0;i<node->num_input_port_sizes;i++)
  {
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  }
  for(i=0;i<node->num_input_pins;i++)
  {
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);

  }
  printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
  for(i=0;i<node->num_output_port_sizes;i++)
  {
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  }
  for(i=0;i<node->num_output_pins;i++)
  {
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node : %s",pin->net->name,pin->node->name);
  }
  
  printf("\n----------zero net----------\n");
  printf("unique_id : %ld name: %s combined: %d \n",net->unique_id,net->name,net->combined);
  printf("driver_pin name : %s num_fanout_pins: %d\n",net->driver_pin->name,net->num_fanout_pins);
  for(i=0;i<net->num_fanout_pins;i++)
  {
    printf("fanout_pins %d : %s",i,net->fanout_pins[i]->name);
  }


   /* vcc_node */
  node=netlist->vcc_node;
  net=netlist->one_net;
  printf("\n--------vcc_node-------\n");
  printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  for(i=0;i<node->num_input_port_sizes;i++)
  {
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  }
  for(i=0;i<node->num_input_pins;i++)
  {
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }
  printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
  for(i=0;i<node->num_output_port_sizes;i++)
  {
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  }
  for(i=0;i<node->num_output_pins;i++)
  {
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }
  
  printf("\n----------one net----------\n");
  printf("unique_id : %ld name: %s combined: %d \n",net->unique_id,net->name,net->combined);
  printf("driver_pin name : %s num_fanout_pins: %d\n",net->driver_pin->name,net->num_fanout_pins);
  for(i=0;i<net->num_fanout_pins;i++)
  {
    printf("fanout_pins %d : %s",i,net->fanout_pins[i]->name);
  }

  /* pad_node */
  node=netlist->pad_node;
  net=netlist->pad_net;
  printf("\n--------pad_node-------\n");
  printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  for(i=0;i<node->num_input_port_sizes;i++)
  {
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  }
  for(i=0;i<node->num_input_pins;i++)
  {
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }
  printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
  for(i=0;i<node->num_output_port_sizes;i++)
  {
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  }
  for(i=0;i<node->num_output_pins;i++)
  {
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  }
  
  printf("\n----------pad net----------\n");
  printf("unique_id : %ld name: %s combined: %d \n",net->unique_id,net->name,net->combined);
  printf("driver_pin name : %s num_fanout_pins: %d\n",net->driver_pin->name,net->num_fanout_pins);
  for(i=0;i<net->num_fanout_pins;i++)
  {
    printf("fanout_pins %d : %s",i,net->fanout_pins[i]->name);
  }

  /* top input nodes */
  printf("\n--------------Printing the top input nodes--------------------------- \n");
  printf("num_top_input_nodes: %d",netlist->num_top_input_nodes);
  for(j=0;j<netlist->num_top_input_nodes;j++)
  {
  	node=netlist->top_input_nodes[j];	
	printf("\ttop input nodes : %d\n",j);
  	printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}
}


  /* top output nodes */
  printf("\n--------------Printing the top output nodes--------------------------- \n");
  printf("num_top_output_nodes: %d",netlist->num_top_output_nodes);
  for(j=0;j<netlist->num_top_output_nodes;j++)
  {
  	node=netlist->top_output_nodes[j];	
	printf("\ttop output nodes : %d\n",j);
  	printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
		net=pin->net;
		printf("\n\t-------printing the related net info-----\n");
		printf("unique_id : %ld name: %s combined: %d \n",net->unique_id,net->name,net->combined);
  printf("driver_pin name : %s num_fanout_pins: %d\n",net->driver_pin->name,net->num_fanout_pins);
  		for(k=0;k<net->num_fanout_pins;k++)
  		{
    		printf("fanout_pins %d : %s",k,net->fanout_pins[k]->name);
  		}
		 function_to_print_node_and_its_pin(net->driver_pin);
  	}
  	printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}

  }


  /* internal nodes */
  printf("\n--------------Printing the internal nodes--------------------------- \n");
  printf("num_internal_nodes: %d",netlist->num_internal_nodes);
  for(j=0;j<netlist->num_internal_nodes;j++)
  {
  	node=netlist->internal_nodes[j];	
	printf("\tinternal nodes : %d\n",j);
  	printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}
}

  /* ff nodes */
  printf("\n--------------Printing the ff nodes--------------------------- \n");
  printf("num_ff_nodes: %d",netlist->num_ff_nodes);
  for(j=0;j<netlist->num_ff_nodes;j++)
  {
  	node=netlist->ff_nodes[j];	
	printf("\tff nodes : %d\n",j);
  	printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s\n",pin->net->name,pin->node->name);
 	}
}
}


void function_to_print_node_and_its_pin(npin_t * temp_pin)  
{
	int i;	
	nnode_t *node;	
	npin_t *pin;

	printf("\n-------Printing the related net driver pin info---------\n");
	node=temp_pin->node;
		
  	printf(" unique_id: %ld   name: %s type: %d\n",node->unique_id,node->name,node->type);
  	printf(" num_input_pins: %d  num_input_port_sizes: %d",node->num_input_pins,node->num_input_port_sizes);
  	for(i=0;i<node->num_input_port_sizes;i++)
  	{
	printf("input_port_sizes %d : %d\n",i,node->input_port_sizes[i]);
  	}
 	 for(i=0;i<node->num_input_pins;i++)
  	{
	pin=node->input_pins[i];	
	printf("input_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
  	}
  	printf(" num_output_pins: %d  num_output_port_sizes: %d",node->num_output_pins,node->num_output_port_sizes);
 	 for(i=0;i<node->num_output_port_sizes;i++)
  	{
	printf("output_port_sizes %d : %d\n",i,node->output_port_sizes[i]);
  	}
  	for(i=0;i<node->num_output_pins;i++)
  	{
	pin=node->output_pins[i];	
	printf("output_pins %d : unique_id : %ld type :%d \n \tname :%s pin_net_idx :%d \n\tpin_node_idx:%d mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
 	}

}	




