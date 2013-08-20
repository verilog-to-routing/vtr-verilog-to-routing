
#include "verilog_writer.h"

/***********************************************************************************************************

Author: Miad Nasr;  August 30, 2012

The code in this file generates the Verilog and SDF files for the post-synthesized circuit. The Verilog file 
can be used to perform functional simulation and the SDF file enables timing simulation of the post-synthesized circuit.

The Verilog file contains instantiated modules of the primitives in the circuit. Currently VPR can generate 
Verilog files for circuits that only contain LUTs , Flip Flops , IOs , Multipliers , and BRAMs. The Verilog 
description of these primitives are in the primitives.v file. To simulate the post-synthesized circuit, one 
must include the generated Verilog file and also the primitives.v Verilog file, in the simulation directory.

If one wants to generate the post-synthesized Verilog file of a circuit that contains a primitive other than 
those mentioned above, he/she should contact the VTR team to have the source code updated. Furthermore to 
perform simulation on that circuit the Verilog description of that new primitive must be appended to the 
primitives.v file as a separate module.

The VTR team can be contacted through the VTR website:
http://code.google.com/p/vtr-verilog-to-routing/

***********************************************************************************************************/

/*The verilog_writer function is the main function that will generate and write to the verilog and SDF files
All the functions declared bellow are called directly or indirectly by verilog_writer()

Basic Description of how verilog_writer() writes the Verilog and SDF files:

First - The name of the clock signal in the circuit is stored in clock_name. the find_clock_name() function 
        searches through all the inputs of the design and returns the name of the clock in the design. The verilog 
        writer currently works with only single clocked circuits

Second - instantiate_top_level() module is called. This function will will traverse through all the inputs and 
        outputs in the circuit and instantiate the list of inputs and inputs in the top level module

Third - instantiate_SDF_header() is a short function that will just write the top level declarations in the SDF file.

Fourth - All the wires in the design are instantiated. This is done in the way demonstrated bellow:
                                                      
                                                      Traverse through all primitives

                                                               -For each primitive traverse through all input ports
                                                                        -For each input port traverse through all input pins and then instantiate the wire

                                                               -For each primitive traverse through all output ports
                                                                        -For each output port traverse through all output pins and then instantiate the wire
         
Fifth - Instantiate_input_interconnect() will instantiate the interconnect modules that connect the inputs to the design to the rest of the primitives.

Sixth - Instantiate_primitive_modules() will instantiate all the primitives in the design. This is how this function works in summary:

                                                      Traverse through all primitives
  
                                                               -For each primitive instantiate the primitive module and instantiate inputs/outputs by traversing
                                                               -For each primitive instantiate the interconnect modules that conect the outputs of that primitive 
                                                                to other primitives
                                                               -Instantiate the SDF block corersponding to this primitive

 
*/

void verilog_writer(void)
{
  FILE *verilog;
  FILE *SDF;
  char * verilog_file_name = (char *)malloc((strlen(blif_circuit_name) + strlen("_post_synthesis.v") + 1) *sizeof(char));/*creating the Verilog file name*/
  char * sdf_file_name = (char *)malloc((strlen(blif_circuit_name) + strlen("_post_synthesis.sdf") + 1) *sizeof(char));/*creating the SDF file name*/
  char *clock_name;/*Will need to store the clock name of the design if any exist*/ 
  assert(verilog_file_name);
  assert(sdf_file_name);
  printf("\nWriting the post-synthesized circuit Verilog and SDF.....\n...");

  sprintf(verilog_file_name, "%s%s", blif_circuit_name, "_post_synthesis.v");
  sprintf(sdf_file_name, "%s%s", blif_circuit_name, "_post_synthesis.sdf");

  /*Openning the post synthesized netlist files*/
  verilog = fopen(verilog_file_name , "w");
  SDF = fopen(sdf_file_name , "w");

  clock_name = find_clock_name();
  instantiate_top_level_module(verilog);
  instantiate_SDF_header(SDF);
  instantiate_wires(verilog);
  instantiate_input_interconnect(verilog , SDF , clock_name);
  instantiate_primitive_modules(verilog,clock_name,SDF);

  fprintf(verilog , "\nendmodule\n");
  fprintf(SDF , ")\n");
  fclose(verilog);
  fclose(SDF);
  printf("Done\n\n");
}

/*The function instantiate_top_level_module instantiates the top level verilog module of the post-synthesized circuit and the list of inputs and outputs to that module*/
void instantiate_top_level_module(FILE *verilog)
{
  int i,place_comma;
  pb_list *temp=NULL;
  pb_list *current;
  char *fixed_name = NULL;
 

  fprintf(verilog , "module %s(\n" , blif_circuit_name);
  place_comma=0;
  for(i=0 ; i<num_blocks ; i++)/*go through all the blocks in the design*/
    {
      if(!strcmp(block[i].pb->pb_graph_node->pb_type->name,"io"))/*if this block is an i/o block*/
        {
          temp = traverse_clb(block[i].pb , temp);/*find all the primitives in this block*/
          for(current=temp ; current!=NULL ; current=current->next)/*traverse through all the primitives in this block*/
            {
              if(place_comma){       /*need this for correct placement of commas and verilog syntax correctness*/
                fprintf(verilog ,"\t,");
              }
              else{
                fprintf(verilog ,"\t");
                place_comma=1;
              }
              fixed_name = fix_name(current->pb->name);/*need to do this, because some names contain characters that cause syntax errors in verilog (e.g. '~' , '^',)*/
              if(!strcmp(logical_block[current->pb->logical_block].model->name,"input")){/*if this primitive is an input pin, declare as input*/

                fprintf(verilog , "input %s\n",fixed_name);/*declaring the inputs*/
	
	     
              }
              else if(!strcmp(logical_block[current->pb->logical_block].model->name,"output")){

                fprintf(verilog , "output %s\n",fixed_name);/*declaring the outputs*/
	      }
              free(fixed_name);
            }
        }
      temp = free_linked_list(temp);
    }
  fprintf(verilog , ");\n\n");

}

void instantiate_SDF_header(FILE *SDF)
{
  fprintf(SDF , "(DELAYFILE\n\t(SDFVERSION \"2.1\")\n\t(DIVIDER /)\n\t(TIMESCALE 1 ps)\n\n ");
}

/*The instantiate_wires function instantiates all the wire in the post-synthesized design*/
void instantiate_wires(FILE *verilog)
{
  int i,j,k;
  pb_list *primitive_list=NULL;
  pb_list *current;
  char *fixed_name = NULL;

  for(i=0; i<num_blocks ; i++)/*go through all the blocks in the design*/
    {
      primitive_list = traverse_clb(block[i].pb , primitive_list);/*find all the primitives in this block*/

      for(current=primitive_list ; current!=NULL ; current=current->next)/*traverse through all the primitives in this block*/
        {
          fixed_name = fix_name(current->pb->name);/*need to do this, because some names contain characters that cause syntax errors in verilog (e.g. '~' , '^',)*/
          for(j=0 ; j<current->pb->pb_graph_node->num_output_ports ; j++)/*go through all the found primitives and instantiate their output wires*/
            {
              for(k=0 ; k<current->pb->pb_graph_node->num_output_pins[j] ; k++)
                {
                 
                    
                      fprintf(verilog , "wire %s_output_%d_%d;\n" , fixed_name , j , k);

                      if(!strcmp(logical_block[current->pb->logical_block].model->name,"input"))/*if that pin is an inpad pin then have to connect the                                        
                                                                                                  inputs to the module to this wire*/
			{
                          fprintf(verilog , "assign %s_output_%d_%d = %s;\n\n",fixed_name , j , k , fixed_name);
                        }

                    
                }
            }
          for(j=0 ; j<current->pb->pb_graph_node->num_input_ports ; j++)/*instantiating the input wire modules for each primitive*/
            {
              for(k=0 ; k<current->pb->pb_graph_node->num_input_pins[j] ; k++)
                {          
		  fprintf(verilog , "wire %s_input_%d_%d;\n" , fixed_name , j , k);
		  if(!strcmp(logical_block[current->pb->logical_block].model->name,"output"))/*if that pin is an outpad then have to connect the input to the outputs 
											       of the top level module*/
		    {
		      fprintf(verilog , "assign %s = %s_input_%d_%d;\n\n",fixed_name , fixed_name , j , k);
		    }     
                }
            }
          free(fixed_name);
        }
      primitive_list = free_linked_list(primitive_list);
    }
}

/*
  The function instantiate_input_interconnect will instantiate the interconnect segments from input pins to the rest of the design.
  This function is necessary because the inputs to the design are not instantiated as a module in verilog.
*/
void instantiate_input_interconnect(FILE *verilog , FILE *SDF , char *clock_name)
{
  int blocks;
  pb_list *current;
  pb_list *primitive_list = NULL;
  conn_list *downhill = NULL;

  for(blocks=0 ; blocks<num_blocks ; blocks++)
    {
      if(strcmp(block[blocks].pb->pb_graph_node->pb_type->name,"io"))/*if this is an io block*/
        {
	  continue;
	}
      primitive_list = traverse_clb(block[blocks].pb , primitive_list);/*find all the primitives in that block*/
      for(current=primitive_list ; current!=NULL ; current=current->next)/*traversing through all the found primitives*/
	{
	  if(strcmp(logical_block[current->pb->logical_block].model->name,"input"))/*if that primitive is an inpad*/
	    {
	      continue;
	    }
	  if(clock_name && !strcmp(current->pb->name,clock_name))/*If this is the clock signal, then don't instantiate an interconnect for it*/
	    {
	      continue;
	    }
	  downhill = find_connected_primitives_downhill(blocks , current->pb , downhill );/*find all the other primitives that it connects to*/
	  
	  /*traverse_linked_list_conn(downhill);*/
	  interconnect_printing(verilog , downhill);/*Print the interconnects modules that connect the inputs to the rest of the design*/
	  SDF_interconnect_delay_printing(SDF , downhill);/*Print the Delays of the interconnect module to the SDF file*/
	  downhill = free_linked_list_conn(downhill);
	  break;
	}
      primitive_list = free_linked_list(primitive_list);
    }
}

/*This function instantiates the primitive verilog modules e.g. LUTs ,Flip Flops, etc.*/
void instantiate_primitive_modules(FILE *fp, char *clock_name , FILE *SDF)
{
  int i,j,k,h;
  pb_list *primitives = NULL;
  pb_list *current;

  int place_comma=0;
  char *truth_table = NULL;
  int inputs_to_lut;
  char *fixed_name = NULL;
  int power;
  for(i=0 ; i<num_blocks ; i++)
    {
      primitives = traverse_clb(block[i].pb , primitives);/*find all the primitives inside block i*/
      for(current=primitives ; current!=NULL ; current=current->next)/*traverse through all the found primitives*/
	{

	  fixed_name = fix_name(current->pb->name);

	  if(!strcmp(logical_block[current->pb->logical_block].model->name,"names"))/*if this primitive is a lut*/
	    {
	      inputs_to_lut = find_number_of_inputs(current->pb);
	      assert((inputs_to_lut >= 3) && (inputs_to_lut <= 7));/*currently the primitives.v file only contains the verilog description for 4LUTs and 6LUTs*/
	      truth_table=load_truth_table(inputs_to_lut,current->pb);/*load_truth_table will find the truth table corresponding to this lut, and store it in "truth_table"*/

	      /*instantiating the 6 input lut module and passing the truth table as parameter*/

	      power = 1 <<  inputs_to_lut;
	      fprintf(fp , "\nLUT_%d #(%d'b%s) lut_%s(", inputs_to_lut , power , truth_table , fixed_name);

	      free(truth_table);

	      j=0;
	      for(k=0 ; k<current->pb->pb_graph_node->num_input_ports ; k++)/*traverse through all the input pins of the lut*/
		{
		  for(h=0 ; h<current->pb->pb_graph_node->num_input_pins[k] ; h++)
		    {
		      if(current->pb->rr_graph[current->pb->pb_graph_node->input_pins[k][h].pin_count_in_cluster].net_num != OPEN)/*check if that pin is used*/
			{
			  if(place_comma)/*need this flag check to properly place the commas*/
			    {
			      fprintf(fp , " , %s_input_%d_%d" , fixed_name , k , j); // j was h
			    }
			  else
			    {
			      fprintf(fp , "%s_input_%d_%d" , fixed_name , k , j); // j was h
			      place_comma=1;
			    }
			  j++;
			}
		    }
		}
	      while(j<inputs_to_lut)/*if (after writing all the inputs to the lut module) there are still some                                                                               
				      inputs unused (i.e.  not all the inputs to the lut are used) place a logic zero instead*/
		{
		  if(place_comma && j)
		    {
		      j++;
		      fprintf(fp , " , 1'b0");
		    }
		  else
		    {
		      j++;
		      fprintf(fp , "1'b0");
		      place_comma=1;
		    }
		}

	      place_comma=0;
	      fprintf(fp , " , %s_output_0_0 );\n\n",fixed_name);/*finaly write the output of the module*/

	      instantiate_interconnect(fp,i,current->pb,SDF);/*this function will instantiate the routing                                                                                    
                                                                          interconnect from the output of this lut to                                                                         
                                                                          the inputs of other luts that it connects to*/
	      sdf_LUT_delay_printing(SDF , current->pb);/*This function will instantiate the internal delays of the lut to the SDF file*/
	    }
	  else if(!strcmp(logical_block[current->pb->logical_block].model->name,"latch"))/*If this primitive is a Flip Flop*/
	    {
	      char *fixed_clock_name;
	      fixed_clock_name = fix_name(clock_name);
	      fprintf(fp , "\nD_Flip_Flop DFF_%s(%s_output_0_0 , %s_input_0_0 , 1'b1 , 1'b1 , %s_output_0_0 );\n",fixed_name,fixed_clock_name,fixed_name,fixed_name);
	      instantiate_interconnect(fp , i , current->pb , SDF);
	      sdf_DFF_delay_printing(SDF , current->pb);
	      free(fixed_clock_name);
	    }
	  else if(!strcmp(logical_block[current->pb->logical_block].model->name,"multiply") || 
		  !strcmp(logical_block[current->pb->logical_block].model->name,"adder")) /*If this primitive is a multiplier or an adder */
	    {
	      /** multipliers and adders are handled quite similarly **/

	      int mult = !strcmp(logical_block[current->pb->logical_block].model->name,"multiply");
	      int num_inputs = logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_pins[0];/*what is the data width*/
	      int i_port , i_pin ;	      
	      place_comma = 0;
	      if (mult)
		fprintf(fp , "\nmult #(%d)%s(" , num_inputs , fixed_name); 
	      else
		fprintf(fp , "\nripple_adder #(%d)%s(" , num_inputs , fixed_name); 
	
	      /*Traversing through all the input ports of this multiplier*/
	      for(i_port=0 ; i_port<logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_ports ; i_port++)
		{
		  if(i_port == 0)/*This if-else statement will make sure that the signals are concatinated in verilog*/
		    {
		      fprintf(fp , "{");
		    }
		  else{
		    fprintf(fp , ",{");
		  }
		  place_comma = 0;
		  /*Traversing through all the input pins of this input port*/
		  for(i_pin=logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_pins[i_port]-1 ; i_pin>=0 ; i_pin--)
		    {
		      /*If this input pin is used*/
		      if(current->pb->rr_graph[current->pb->pb_graph_node->input_pins[i_port][i_pin].pin_count_in_cluster].net_num != OPEN)
			{
			  if(place_comma)/*need this flag check to properly place the commas*/
			    {
			      fprintf(fp , " , %s_input_%d_%d" , fixed_name , i_port , i_pin);
			    }
			  else
			    {
			      fprintf(fp , "%s_input_%d_%d" , fixed_name , i_port , i_pin);
			      place_comma=1;
			    }
			
			}
		      else{/*If this pin was not used, then instantiate logic zero*/
			if(place_comma)
			  {
			    fprintf(fp , ", 1'b0");
			  }
			else{
			  fprintf(fp , " 1'b0");
			  place_comma = 1;
			}
		      }
		    }
		  fprintf(fp , "}");/*End concatination*/
		}
	      /*Traversing through all the output ports of this multiplier*/
	      for(i_port=0 ; i_port<logical_block[current->pb->logical_block].pb->pb_graph_node->num_output_ports ; i_port++)
		{
		  fprintf(fp , ",{");/*Since there is only a single output port for multipliers we only need to concatinate once*/
		  place_comma = 0;
		  /*Traversing through all the output pins of this output port*/
		  for(i_pin=logical_block[current->pb->logical_block].pb->pb_graph_node->num_output_pins[i_port]-1 ; i_pin>=0 ; i_pin--)
		    {
		      if(place_comma)
			{
			  fprintf(fp , ", %s_output_%d_%d" , fixed_name , i_port , i_pin);
			}
		      else{
			fprintf(fp , " %s_output_%d_%d" , fixed_name , i_port , i_pin);
			place_comma = 1;
		      }
		    }
		  fprintf(fp , "}");/*End concatination*/
		}
	      fprintf(fp , ");\n\n");
	      instantiate_interconnect(fp , i , current->pb , SDF);
	      if (mult)
		SDF_Mult_delay_printing(SDF, current->pb);
	      else
		SDF_Adder_delay_printing(SDF, current->pb);
	    }
	  else if(!strcmp(logical_block[current->pb->logical_block].model->name,"single_port_ram")){/*If this primitive is a single port RAM block*/
	    
	    char *fixed_clock_name;
	    int i_port,i_pin;
	    int data_width,addr_width;
	    
	    data_width = logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_pins[1];/*the data_width (word width) is the number of inputs in the data port*/
	    addr_width = logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_pins[0];/*the addr_width (address width) is the number of inputs in the addr port*/

	    fprintf(fp , "\nsingle_port_ram #(%d,%d)%s(" , addr_width , data_width , fixed_name);/*Passing the address width and data width as a verilog parameter*/
	    /*Traversign through all the input ports of this single_port_ram*/
	    for(i_port=0 ; i_port<logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_ports ; i_port++)
	      {
		if(i_port == 0)/*This if-else statement will make sure that the signals are concatinated in verilog*/
		  {
		    fprintf(fp , "{");
		  }
		else{
		  fprintf(fp , ",{");
		}
		place_comma = 0;
		/*Traversing through all the input pins of this input port*/
		for(i_pin=logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_pins[i_port]-1 ; i_pin>=0 ; i_pin--)
		  {
		    /*If this input pin is used*/
		    if(current->pb->rr_graph[current->pb->pb_graph_node->input_pins[i_port][i_pin].pin_count_in_cluster].net_num != OPEN)
		      {
			if(place_comma)/*need this flag check to properly place the commas*/
			  {
			    fprintf(fp , " , %s_input_%d_%d" , fixed_name , i_port , i_pin);
			  }
			else
			  {
			    fprintf(fp , "%s_input_%d_%d" , fixed_name , i_port , i_pin);
			    place_comma=1;
			  }
		      }
		    else{/*If this input pin is not used then instantiate a logic zero*/
		      if(place_comma)
			{
			  fprintf(fp , ", 1'b0");
			}
		      else{
			fprintf(fp , " 1'b0");
			place_comma = 1;
		      }
		    }
		  }
		fprintf(fp , "}");/*End concatination*/
	      }
	    /*Traversign through all the output ports of this single_port_ram*/
	    for(i_port=0 ; i_port<logical_block[current->pb->logical_block].pb->pb_graph_node->num_output_ports ; i_port++)
	      {
		fprintf(fp , ",{");/*Since there is only a single output port for single_port_rams we only need to concatinate once*/
		place_comma = 0;
		/*Traverse through all the output pins of this output port*/
		for(i_pin=logical_block[current->pb->logical_block].pb->pb_graph_node->num_output_pins[i_port]-1 ; i_pin>=0 ; i_pin--)
		  {
		    if(place_comma)
		      {
			fprintf(fp , ", %s_output_%d_%d" , fixed_name , i_port , i_pin);
		      }
		    else{
		      fprintf(fp , " %s_output_%d_%d" , fixed_name , i_port , i_pin);
		      place_comma = 1;
		    }
		  }
		fprintf(fp , "}");/*End concatination*/
	      }
	    fixed_clock_name = fix_name(clock_name);/*Must also instantiate the clock signal to the RAM block*/
	    fprintf(fp , ", %s_output_0_0" , fixed_clock_name);
	    fprintf(fp , ");\n\n");
	    free(fixed_clock_name);
	    instantiate_interconnect(fp , i , current->pb , SDF);
	    SDF_ram_single_port_delay_printing(SDF , current->pb);
	  }
	  else if(!strcmp(logical_block[current->pb->logical_block].model->name,"dual_port_ram")){/*If this primitive is a dual_port_ram*/
	    
	    int data1_width,data2_width,addr1_width,addr2_width,i_port,i_pin;
	    char *fixed_clock_name;

	    /*The Input ports of dual port rams are in this order:
	     0-addr1
	     1-addr2
	     2-data1
	     3-data2
	     4-we1
	     5-we2

	    The verilog primitives module only accepts instatiation in this order else the simulation will give error or give incorrect simulation results*/

	    data1_width = current->pb->pb_graph_node->num_input_pins[2];/*the data_width (word width) for the first port, is the number of inputs in the first data port*/
	    addr1_width = current->pb->pb_graph_node->num_input_pins[0];/*the addr_width (address width) for the first port, is the number of inputs in the first addr port*/
	    data2_width = current->pb->pb_graph_node->num_input_pins[3];/*the data_width (word width) for the second port, is the number of inputs in the second data port*/
	    addr2_width = current->pb->pb_graph_node->num_input_pins[1];/*the addr_width (address width) for the second port is the number of inputs in the second addr port*/
	    
	    fprintf(fp , "\ndual_port_ram #(%d,%d,%d,%d)%s(" , addr1_width , data1_width , addr2_width , data2_width , fixed_name);/*passing the addr_wdith and data_wdith for both ports as verilog parameter*/
	   
	    for(i_port=0 ; i_port<logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_ports ; i_port++)
              {
                if(i_port == 0)
                  {
                    fprintf(fp , "{");
                  }
                else{
                  fprintf(fp , ",{");
                }
                place_comma = 0;
		/*Traversign through all the input pins of this input port*/
                for(i_pin=logical_block[current->pb->logical_block].pb->pb_graph_node->num_input_pins[i_port]-1 ; i_pin>=0 ; i_pin--)
                  {
		    /*If this pin is used*/
                    if(current->pb->rr_graph[current->pb->pb_graph_node->input_pins[i_port][i_pin].pin_count_in_cluster].net_num != OPEN)
                      {
                        if(place_comma)/*need this flag check to properly place the commas*/
                          {
                            fprintf(fp , " , %s_input_%d_%d" , fixed_name , i_port , i_pin);
                          }
                        else
                          {
                            fprintf(fp , "%s_input_%d_%d" , fixed_name , i_port , i_pin);
                            place_comma=1;
                          }
                      }
                    else{/*If this pin is not used, then instantiate logic zero instead*/
                      if(place_comma)
                        {
                          fprintf(fp , ", 1'b0");
                        }
                      else{
                        fprintf(fp , " 1'b0");
                        place_comma = 1;
                      }
                    }
                  }
                fprintf(fp , "}");/*End concatination*/
              }
	    /*Traversign through all the output ports of this dual_port_ram*/
	    for(i_port=0 ; i_port<logical_block[current->pb->logical_block].pb->pb_graph_node->num_output_ports ; i_port++)
              {
                fprintf(fp , ",{");
                place_comma = 0;
		/*Traverse through all the output pins of this output port*/
                for(i_pin=logical_block[current->pb->logical_block].pb->pb_graph_node->num_output_pins[i_port]-1 ; i_pin>=0 ; i_pin--)
                  {
                    if(place_comma)
                      {
                        fprintf(fp , ", %s_output_%d_%d" , fixed_name , i_port , i_pin);
                      }
                    else{
                      fprintf(fp , " %s_output_%d_%d" , fixed_name , i_port , i_pin);
                      place_comma = 1;
                    }
                  }
                fprintf(fp , "}");
              }
            fixed_clock_name = fix_name(clock_name);/*Must also instantiate the clock */
            fprintf(fp , ", %s_output_0_0" , fixed_clock_name);
            fprintf(fp , ");\n\n");
            free(fixed_clock_name);
            instantiate_interconnect(fp , i , current->pb , SDF);
            SDF_ram_dual_port_delay_printing(SDF , current->pb);

	  }
	  else if(strcmp(logical_block[current->pb->logical_block].model->name,"input") && strcmp(logical_block[current->pb->logical_block].model->name,"output"))
	    /*If this primitive is anything else, but an input or an output*/
	    {
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"Failed to generate post-synthesized verilog and sdf files. Primitive %s is unknown.\n"
				"\nAcceptable primitives are: LUTs, Flip Flops, IOs, Adders, Rams, and Multiplier blocks.\n"
				"\nTo generate the post synthesized verilog and SDF files successfully, you must append the "
				"verilog code for the %s to the primitives.v file, and contact the VPR developers team on "
				"the website: http://code.google.com/p/vtr-verilog-to-routing/ to update the VPR source code to handle the new primitive. \n", 
				logical_block[current->pb->logical_block].model->name , logical_block[current->pb->logical_block].model->name);
	    }
	  free(fixed_name);
	}
      primitives = free_linked_list(primitives);
    }
}

/*This function instantiates the interconnect modules that connect from the output pins of the primitive "pb" to whatever it connects to block_num is the block number that the pb resides in*/
void instantiate_interconnect(FILE *verilog , int block_num , t_pb *pb , FILE *SDF)
{
  conn_list *downhill_connections = NULL;

  downhill_connections = find_connected_primitives_downhill(block_num , pb , downhill_connections);/*find all the other luts that the lut corresponding to "pb", connects to*/

  interconnect_printing(verilog , downhill_connections);
  SDF_interconnect_delay_printing(SDF , downhill_connections);
  downhill_connections = free_linked_list_conn(downhill_connections);
}

/*load_truth_table returns the truth table of the LUT corresponding to "pb"
inputs: total number of inputs that the LUT has.
pb: the t_pb data structure corresponding to the LUT*/

char *load_truth_table(int inputs , t_pb *pb)
{
  int number_of_dont_cares=0;
  int tries,shift,which_row,i,j;
  int possibles = 1 << inputs;
  char *tt_row_blif;
  char *possible_row = (char *)malloc(inputs+1 * sizeof(char));
  char *tt = (char *)malloc((possibles+1) * sizeof(char));
  struct s_linked_vptr *current;
  int number_of_used_inputs_to_lut;
  char set_to;
  assert(possible_row);
  assert(tt);
 
  if (logical_block[pb->logical_block].truth_table) { // ??? janders ???? how can you have a LUT without a truth table???
    tt_row_blif =  (char *)logical_block[pb->logical_block].truth_table->data_vptr;
  }
  else { // must be producing a GROUND
    for (i = 0; i < possibles; i++)
      tt[i] = '0';
    tt[possibles] = '\0';
    free(possible_row);
    return tt;
  }
  set_to = tt_row_blif[strlen(tt_row_blif)-1];

  /*filling the truth table with the state that is opposite to the output state in the blif truth table*/
  if(set_to =='1')
    {
      for(i=0 ; i<possibles ; i++)
        {
          tt[i]='0';
        }
    }
  else{
    for(i=0 ; i<possibles ; i++)
      {
        tt[i]='1';
      }
  }
  tt[possibles] = '\0';/*null terminating the tt string*/
  /*The code bellow will populate the tt[] array with the truth table of the LUT with "pb"
    
    The truth table is stored as a linked list in the VPR. Each node in the linked list stores one line of the truth table. This line is in the exact same format as in the blif file. Each line of the truth table is read by traversing through each node of the linked list. The code bellow then parses each line of the truth table. That line might not have all the inputs to the lut specified, or there could be don't cares in that line of the truth table. at the end we need the output value of the LUT for every possible combination of inputs, which is not given in the blif truth table.

  The code bellow first counts the number of don't cares in that line of the truth table. It also creates a temporary string "possible_row". and there are n=2^(number of don't cares) possibilities in a particular line of the truth table So in a separate loop we find the nth possibility of the truth table*/

  for(current=logical_block[pb->logical_block].truth_table ; current!=NULL ; current=current->next)/*traversing through all the lines of the blif truth table*/
    {

      tt_row_blif=(char *)current->data_vptr;/*tt_row_blif stores the current truth table line*/


      /*counting number of don't cares in this line of the blif truth table*/
      number_of_used_inputs_to_lut = strlen(tt_row_blif)-2;/*used*/
      if(number_of_used_inputs_to_lut == 0)/*If this is a constant generator, then the truth table only contains the constant value that it generates.*/
        {
          for(i=0 ; i<possibles ; i++)
            {
              tt[i] = tt_row_blif[1];
            }
        }
      else{
        for(i=0; i<inputs ; i++)
          {
            if(tt_row_blif[i] == '-')
              {
                number_of_dont_cares++;
              }
          }
        tries = 1 << number_of_dont_cares;/*how many possibilities are there for this line of the blif truth table*/

        for(i=0 ; i<(tries) ; i++)/*traverse through all the possibilities for this line of the blif truth table*/
          {
            shift=number_of_dont_cares-1;
            for(j=0 ; j<number_of_used_inputs_to_lut ; j++)/*going through all the columns of the blif truth table*/
              {
                if(tt_row_blif[j]=='-')/*if this column represents a don't care*/
                  {
                    /*creating a temporary truth table row by taking different values of the possiblity 'i' depending on the shift value*/
                    if(((i>>shift) & 0x1) == 1)
                      {
                        possible_row[j] = '1';
                      }
                    else{
                      possible_row[j] = '0';
                    }
                    shift--;/*reduce the shift value by one*/
                  }
                else
                  {
                    possible_row[j]=tt_row_blif[j];
                  }
              }
            possible_row[j]=0x0;/*null terminate the temporary truth table row*/
            which_row = find_index(possible_row,inputs);/*find index returns the index of the 64bit truth table that this temporary truth table row corresponds to*/
            tt[possibles-1-which_row] = set_to;
          }
      }
      number_of_dont_cares = 0;
    }
  free(possible_row);
  return(tt);
}

int find_index(char *row,int inputs)/*returns the index of the 64bit truth table that this temporary truth table row corresponds to*/
{
  int index=0;/*initially setting index to 0*/
  int or_=0x1;
  int i,length;

  for(i=strlen(row)-1 ; i>=0 ; i--)/*traverse through the columns of this truth table row*/
    {
      if(row[i] == '1')
	{
	  index |= or_;/*if this column is logic 1, then set this column in index to logic 1.*/
	}
      or_ = or_ << 1;
    }
  length = strlen(row);
  if(length<inputs)/*if the number of used inputs to the lut is less than the total inputs to the lut*/
    {
      index = index << (inputs-(strlen(row)));/*shift the index value by the difference*/
    }
  return(index);

}

/*The names of some primitives contain certain characters that would cause syntax errors in Verilog (e.g. '^' , '~' , '[' , etc. ). This function returns a new string
  with those illegal characters removed and replaced with '_'*/
char *fix_name(char *name)
{
  int i;

  char *new_;
  new_=my_strdup(name);

  for(i=0 ; new_[i]!='\0' ; i++)
    {
      if(new_[i]=='^' || (int)new_[i]<48 || ((int)new_[i]>57 && (int)new_[i]<65) || ((int)new_[i]>90 && (int)new_[i]<97) || (int)new_[i]>122)
        {
          new_[i]='_';
        }
    }
  return(new_);
}

/*This function finds the number of inputs to a primitive.*/
int find_number_of_inputs(t_pb *pb)
{
  int i,j,count=0;
  for(i=0 ; i<pb->pb_graph_node->num_input_ports ; i++)
    {
      for(j=0 ; j<pb->pb_graph_node->num_input_pins[i] ; j++)
        {
          count++;
        }
    }
  return(count);
}

/*This function is a utility function called by intantiate_interconnect*/
void interconnect_printing(FILE *fp , conn_list *downhill)
{
  char *fixed_name1;
  char *fixed_name2;
  conn_list *connections;
  int port_number_out=-1,port_number_in=-1,i;  

    for(connections=downhill ; connections!=NULL ; connections=connections->next)/*traverse through all the connected primitives and instantiate a routing interconect module*/
    {
      fixed_name1 = fix_name(connections->driver_pb->name);
      fixed_name2 = fix_name(connections->load_pb->name);

      for(i=0 ; i<connections->load_pin->parent_node->num_input_ports ; i++)
	{
	  if(!strcmp(connections->load_pin->parent_node->input_pins[i][0].port->name,connections->load_pin->port->name))
	    {
	      port_number_out = i;
	    }
	}
      for(i=0 ; i<connections->driver_pin->parent_node->num_output_ports ; i++)
        {
          if(!strcmp(connections->driver_pin->parent_node->output_pins[i][0].port->name,connections->driver_pin->port->name))
            {
              port_number_in = i;
            }
        }     
      assert(port_number_out >= 0 && port_number_in >= 0);

      fprintf(fp , "interconnect routing_segment_%s_output_%d_%d_to_%s_input_%d_%d( %s_output_%d_%d , %s_input_%d_%d );\n",
              fixed_name1 , port_number_in/*connections->driver_pin->port->port_index_by_type*/ , connections->driver_pin->pin_number,
              fixed_name2 , port_number_out , connections->load_pin->pin_number,
              fixed_name1 , port_number_in/*connections->driver_pin->port->port_index_by_type*/ , connections->driver_pin->pin_number,
              fixed_name2 , port_number_out , connections->load_pin->pin_number);
      
      free(fixed_name1);
      free(fixed_name2);
    }
}

/*This funciton will instantiate the SDF cell that contains the delay information of the Verilog interconnect modules*/
void SDF_interconnect_delay_printing(FILE *SDF , conn_list *downhill)
{
  conn_list *connections;
  char *fixed_name1;
  char *fixed_name2;
  float internal_delay;
  int del;
  int port_number_out=-1,port_number_in=-1,i;  

   for(connections=downhill ; connections!=NULL ; connections=connections->next)/*traverse through all the connected primitives and instantiate a routing interconect module*/
    {
      fixed_name1 = fix_name(connections->driver_pb->name);
      fixed_name2 = fix_name(connections->load_pb->name);

      for(i=0 ; i<connections->load_pin->parent_node->num_input_ports ; i++)
	{
	  if(!strcmp(connections->load_pin->parent_node->input_pins[i][0].port->name,connections->load_pin->port->name))
	    {
	      port_number_out = i;
	    }
	}
      for(i=0 ; i<connections->driver_pin->parent_node->num_output_ports ; i++)
        {
          if(!strcmp(connections->driver_pin->parent_node->output_pins[i][0].port->name,connections->driver_pin->port->name))
            {
              port_number_in = i;
            }
        }     
      internal_delay = connections->driver_to_load_delay;
      internal_delay = internal_delay * 1000000000000.00; /*converting the delay from seconds to picoseconds*/
      internal_delay = internal_delay + 0.5;              /*Rounding the delay to the nearset picosecond*/
      del = (int)internal_delay;

      fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"interconnect\")\n\t(INSTANCE inst/routing_segment_%s_output_%d_%d_to_%s_input_%d_%d)\n" ,
	      fixed_name1 , port_number_in/*connections->driver_pin->port->port_index_by_type*/ , connections->driver_pin->pin_number,
              fixed_name2 , port_number_out , connections->load_pin->pin_number);
      fprintf(SDF , "\t\t(DELAY\n\t\t(ABSOLUTE\n\t\t\t(IOPATH datain dataout (%d:%d:%d)(%d:%d:%d))\n\t\t)\n\t\t)\n\t)\n" ,
              del , del , del , del , del , del);

      free(fixed_name1);
      free(fixed_name2);

    }
}

/*This function instantiates the SDF cell that contains the delay information of a LUT*/
void sdf_LUT_delay_printing(FILE *SDF , t_pb *pb)
{
  char *fixed_name;
  int j,pin_count;
  float internal_delay;
  int del;
  int logical_block_index = pb->logical_block;
  int record = 0;

  fixed_name = fix_name(pb->name);

  for(j=0 ; j < pb->pb_graph_node->num_input_pins[0] ; j++)/*Assuming that LUTs have a single input port*/
  {
     if (logical_block[logical_block_index].input_nets[0][j] != OPEN) 
      {
	  
	  int q;
	  pin_count = OPEN;
	  for (q = 0; q < pb->pb_graph_node->num_input_pins[0]; q++)
	  {
	    pin_count = pb->pb_graph_node->input_pins[0][q].pin_count_in_cluster;
	    if (logical_block[logical_block_index].pb->rr_graph[pin_count].net_num == logical_block[logical_block_index].input_nets[0][j])
	      break;
	  }
	  
	  assert(q != pb->pb_graph_node->num_input_pins[0]);

          internal_delay = pb->rr_graph[pin_count].tnode->out_edges->Tdel;
          internal_delay = internal_delay * 1000000000000.00;/*converting the delay from seconds to picoseconds*/
          internal_delay = internal_delay + 0.5;             /*Rounding the delay to the nearset picosecond*/
          del = (int)internal_delay;

	  if (!record) { // print the SDF record header
	    fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"LUT_%d\")\n\t(INSTANCE inst/lut_%s)\n\t\t(DELAY\n\t\t(ABSOLUTE\n" , find_number_of_inputs(pb) , fixed_name);
	    record = 1;
	  }

          fprintf(SDF , "\t\t\t(IOPATH inter%d/datain inter%d/dataout (%d:%d:%d)(%d:%d:%d))\n" , j , j , del , del , del , del , del , del);
        }
    }
  if (record) 
    fprintf(SDF , "\t\t)\n\t\t)\n\t)\n");
  free(fixed_name);
}

/*This function instantiates the SdF cell that contains the delay information of a Flip Flop*/
void sdf_DFF_delay_printing(FILE *SDF , t_pb *pb)
{
  char *fixed_name;
  float internal_delay;
  int del,pin_count;

  fixed_name = fix_name(pb->name);
  fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"D_Flip_Flop\")\n\t(INSTANCE inst/DFF_%s)\n\t\t(DELAY\n\t\t(ABSOLUTE\n" , fixed_name);

  pin_count = pb->pb_graph_node->output_pins[0][0].pin_count_in_cluster; // the Q output of the FF (which has only a single output port and pin)
  assert(pb->rr_graph[pin_count].tnode);
  internal_delay = pb->rr_graph[pin_count].tnode->T_arr * 1.0E12 + 0.5;
  del = (int)internal_delay;

  fprintf(SDF , "\t\t\t(IOPATH (posedge clock) Q (%d:%d:%d)(%d:%d:%d))\n" , del , del , del , del , del , del);
  fprintf(SDF , "\t\t)\n\t\t)\n\t)\n");
  free(fixed_name);
}

void SDF_Mult_delay_printing(FILE *SDF , t_pb *pb)
{
  char *fixed_name;
  int pin_count;
  float internal_delay;
  int del;

  fixed_name = fix_name(pb->name);
  fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"mult\")\n\t(INSTANCE inst/%s)\n\t\t(DELAY\n\t\t(ABSOLUTE\n" ,  fixed_name);


  pin_count = pb->pb_graph_node->input_pins[0][0].pin_count_in_cluster;
  assert(pb->rr_graph[pin_count].tnode);
  internal_delay = pb->rr_graph[pin_count].tnode->out_edges->Tdel;
  internal_delay = internal_delay * 1000000000000.00;/*converting the delay from seconds to picoseconds*/
  internal_delay = internal_delay + 0.5;             /*Rounding the delay to the nearset picosecond*/
  del = (int)internal_delay;
  fprintf(SDF , "\t\t\t(IOPATH delay/A delay/B (%d:%d:%d)(%d:%d:%d))\n" , del , del , del , del , del , del);
  
  pin_count = pb->pb_graph_node->input_pins[1][0].pin_count_in_cluster;
  assert(pb->rr_graph[pin_count].tnode);
  internal_delay = pb->rr_graph[pin_count].tnode->out_edges->Tdel;
  internal_delay = internal_delay * 1.0E12;/*converting the delay from seconds to picoseconds*/
  internal_delay = internal_delay + 0.5;             /*Rounding the delay to the nearset picosecond*/
  del = (int)internal_delay;
  fprintf(SDF , "\t\t\t(IOPATH delay2/A delay2/B (%d:%d:%d)(%d:%d:%d))\n" , del , del , del , del , del , del);
  
  fprintf(SDF , "\t\t)\n\t\t)\n\t)\n");
  free(fixed_name);
  
}

void SDF_Adder_delay_printing(FILE *SDF , t_pb *pb)
{
  int i, j, k;
  int total_input_ports = pb->pb_graph_node->num_input_ports;
  int pin_count;
  char *fixed_name;
  int record = 0;
  
  fixed_name = fix_name(pb->name);

  for (i = 0; i < total_input_ports; i++) {

    for (j = 0; j < pb->pb_graph_node->num_input_pins[i]; j++) {

      pin_count = pb->pb_graph_node->input_pins[i][j].pin_count_in_cluster;
      
      t_tnode* tNodeInput = pb->rr_graph[pin_count].tnode; // source pin of timing arc

      if (!tNodeInput)
	continue; // no timing information on pin
      
      for (k = 0; k < tNodeInput->num_edges; k++) {   

	t_tnode tNodeOutput = tnode[tNodeInput->out_edges[k].to_node]; // dest pin of timing arc
	int del = tNodeInput->out_edges[k].Tdel * 1.0E12 + 0.5;

	if (!record) { // print the SDF record header
	  fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"ripple_adder\")\n\t(INSTANCE inst/%s)\n\t\t(DELAY\n\t\t(ABSOLUTE\n" ,  fixed_name);
	  record = 1;
	}

	fprintf(SDF , "\t\t\t(IOPATH %s %s (%d:%d:%d)(%d:%d:%d))\n" , tNodeInput->pb_graph_pin->port->name, 
		tNodeOutput.pb_graph_pin->port->name,
		del , del , del , del , del , del);

      }
      j =  pb->pb_graph_node->num_input_pins[i]; // skip to the next port -- JANDERS to fix
      // what we really want to do here is find the max delay between any pin of the input port, to any pin of the output port
    }
  }

  if (record)
    fprintf(SDF , "\t\t)\n\t\t)\n\t)\n");
  free(fixed_name);
}


void SDF_ram_single_port_delay_printing(FILE *SDF , t_pb *pb)
{
  //  int num_inputs;
  char *fixed_name;
  int pin_count;
  float internal_delay;
  int del;

  //  num_inputs = pb->pb_graph_node->num_input_pins[0];
  fixed_name = fix_name(pb->name);
  fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"single_port_ram\")\n\t(INSTANCE inst/%s)\n\t\t(DELAY\n\t\t(ABSOLUTE\n" ,  fixed_name);

  pin_count = pb->pb_graph_node->output_pins[0][0].pin_count_in_cluster; // an output pin of the RAM
  assert(pb->rr_graph[pin_count].tnode);
  internal_delay = pb->rr_graph[pin_count].tnode->T_arr * 1.0E12 + 0.5;
  del = (int)internal_delay;

  fprintf(SDF , "\t\t\t(IOPATH (posedge clock) out (%d:%d:%d)(%d:%d:%d))" , del , del , del , del , del , del);

  fprintf(SDF , "\t\t)\n\t\t)\n\t)\n");
  free(fixed_name);

}

void SDF_ram_dual_port_delay_printing(FILE *SDF , t_pb *pb)
{
  //  int num_inputs;
  char *fixed_name;
  int pin_count;
  float internal_delay;
  int del;

  //  num_inputs = pb->pb_graph_node->num_input_pins[0];
  fixed_name = fix_name(pb->name);
  fprintf(SDF , "\t(CELL\n\t(CELLTYPE \"dual_port_ram\")\n\t(INSTANCE inst/%s)\n\t\t(DELAY\n\t\t(ABSOLUTE\n" ,  fixed_name);

  pin_count = pb->pb_graph_node->output_pins[0][0].pin_count_in_cluster; // an output pin of the RAM
  assert(pb->rr_graph[pin_count].tnode);
  internal_delay = pb->rr_graph[pin_count].tnode->T_arr * 1.0E12 + 0.5;
  del = (int)internal_delay;

  fprintf(SDF , "\t\t\t(IOPATH (posedge clock) out1 (%d:%d:%d)(%d:%d:%d))" , del , del , del , del , del , del);
  fprintf(SDF , "\t\t\t(IOPATH (posedge clock) out2 (%d:%d:%d)(%d:%d:%d))" , del , del , del , del , del , del);

  fprintf(SDF , "\t\t)\n\t\t)\n\t)\n");
  free(fixed_name);
}

char *find_clock_name(void)
{
  unsigned int j;
  char *clock_in_the_design=NULL;
  int clocks = 0;
  for(j=0 ; j<g_clbs_nlist.net.size() ; j++)/*Doing this to find the clock name in the design and storing it in clock_,*/
    {
      /* TODO fix this Hack: Currently detect if a g_clbs_nlist is a clock using global and not vcc/gnd, need better way */
      if(g_clbs_nlist.net[j].is_global == TRUE && strcmp(g_clbs_nlist.net[j].name, "gnd") != 0 
		  && strcmp(g_clbs_nlist.net[j].name, "vcc") != 0)
	{
	  clock_in_the_design = g_clbs_nlist.net[j].name;
	  clocks++;
	}
    }
  if (clocks > 1) {
    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
		"The post-layout netlist generator presently handles single-clock designs only.  Your design contains %d clocks. \n"
		"Future VTR releases may support post-layout netlist generation for multi-clock designs.\n", clocks);
  }
  return(clock_in_the_design);
}
/*The traverse_clb function returns a linked list of all the primitives inside a complex block.                                                                                   
  
  These primitives may be LUTs , FlipFlops , etc.
  block_num: the block number for the complex block.
  pb: The t_pb data structure corresponding to the complex block. (i.e block[block_num].pb)
  prim_list: A pin_list pointer corresponding to the head of the linked list. The function will populate this head pointer.*/

pb_list *traverse_clb(t_pb *pb , pb_list *prim_list)
{
  int i,j;
  const t_pb_type *pb_type;
  if(pb == NULL || pb->name == NULL) {  /*Reached a pb with no content*/
    return(prim_list);
  }
  
  pb_type = pb->pb_graph_node->pb_type;

  if(pb->child_pbs == NULL) {/*reached a primitive*/
    prim_list = insert_to_linked_list(pb , prim_list);   /*add the primitive to the linked list*/
  }

  if(pb_type->num_modes > 0) {                                              /*Traversing through the children of the pb*/
    for(i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
      for(j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
	prim_list = traverse_clb(&(pb->child_pbs[i][j]) , prim_list);
      }
    }  
  }

  return(prim_list);

}

/*The find_connected_primitives_downhill function will return a linked list of all the primitives that a particular primitive connects to.

  block_num: the block number of the complex block that the primitive resides in.
  pb: A pointer to the t_pb data structure that represents the primitive (not the complex block).
  list: A head pointer to the start of a linked list. This function will populate the linked list. The linked list can be empty (i.e list=NULL)
  or contain other primitives.*/

conn_list *find_connected_primitives_downhill(int block_num , t_pb *pb , conn_list*list)
{
  int i,j,q,r;
  unsigned int k;
  int total_output_pins;
  int pin_number , port_number_out=-1 ,  pin_number_out , starting_block , next_block , vpck_net , pin_count;
  float delay , start_delay , end_delay;
 
  char *temp_port_name = (char *)malloc(1000 * sizeof(char));
  int total_output_ports = pb->pb_graph_node->num_output_ports; 
  int model_port_index;

  assert(temp_port_name);

  /*iterates through all output pins of the primitive "pb", and finds the other primitive and pin they connect to*/
  for(i=0 ; i < total_output_ports ; i++)
  {
      total_output_pins = pb->pb_graph_node->num_output_pins[i];
      for(j=0 ; j < total_output_pins ; j++)
      {	  
	  
	  model_port_index =  pb->pb_graph_node->output_pins[i][j].port->model_port->index;

          pin_number = pb->pb_graph_node->output_pins[i][j].pin_count_in_cluster;  /*pin count of the output pin*/
          starting_block = pb->logical_block;  /*logical block index for the source primitive*/

	  vpck_net = logical_block[starting_block].output_nets[model_port_index][j]; /*The index for the g_atoms_nlist.net struct corresponding to this source to sink(s) connections*/
	 
	  if (pb->rr_graph[pin_number].net_num != OPEN) /* If this output pin is used*/
	  { /*Then we will use the logical_block netlist method for finding the connectivity and timing information*/
	      if(vpck_net != OPEN)
	      {

		for(k=1 ; k<g_atoms_nlist.net[vpck_net].pins.size() ; k++)/*traversing through all the sink primitives that the source primitive connects to*/
		  {
			  next_block = g_atoms_nlist.net[vpck_net].pins[k].block;/*next_blk holds the logical block index for the primitive that the source primitive connects to*/

#if 0

		      // we now need to map between the port on the logical block to the port on the physical block
		      t_pb_graph_pin* iPin = get_pb_graph_node_pin_from_g_atoms_nlist_net(vpck_net, k);
		      port_number_out = iPin->port->index;

		      // because of possible pin swaps, we need to find the physical on load block that
		      // has the correct attached net
		      for (q = 0; q <  logical_block[next_block].pb->pb_graph_node->num_input_pins[port_number_out]; q++) 
		      {
			int physical_pin_pos = logical_block[next_block].pb->pb_graph_node->input_pins[port_number_out][q].pin_count_in_cluster;
			if (logical_block[next_block].pb->rr_graph[physical_pin_pos].net_num == vpck_net)
			  break;
		      }
		      
		      assert(q != logical_block[next_block].pb->pb_graph_node->num_input_pins[port_number_out]);

		      pin_number_out = q;
#endif

		      port_number_out = pin_number_out = -1;

		      for (r = 0; r < logical_block[next_block].pb->pb_graph_node->num_input_ports; r++) {
			 for (q = 0; q < logical_block[next_block].pb->pb_graph_node->num_input_pins[r]; q++) {
			   int physical_pin_pos = logical_block[next_block].pb->pb_graph_node->input_pins[r][q].pin_count_in_cluster;
			   if (logical_block[next_block].pb->rr_graph[physical_pin_pos].net_num == vpck_net) {
			     port_number_out = r;
			     pin_number_out = q;
			     r = logical_block[next_block].pb->pb_graph_node->num_input_ports;
			     break;
			   }
			 }
		      }

		      assert(port_number_out != -1);
		      assert(pin_number_out != -1);
		      
			  int unswapped_pin_number =  g_atoms_nlist.net[vpck_net].pins[k].block_pin;

		      pin_count = logical_block[next_block].pb->pb_graph_node->input_pins[port_number_out][pin_number_out].pin_count_in_cluster;/*pin count for the sink pin*/
		      assert(logical_block[next_block].pb->rr_graph[pin_count].tnode);
		      
		      start_delay = pb->rr_graph[pin_number].tnode->T_arr; /*The arrival time of the source pin*/		
		      end_delay = logical_block[next_block].pb->rr_graph[pin_count].tnode->T_arr;/*The arrival time of the sink pin*/
		      delay = end_delay - start_delay;   /*The difference of start and end arrival times is the delay for going from the source to sink pin*/		
		      list=insert_to_linked_list_conn(pb , logical_block[next_block].pb , 
						      &pb->pb_graph_node->output_pins[i][j] , 
						      &logical_block[next_block].pb->pb_graph_node->input_pins[port_number_out][unswapped_pin_number] ,
						      delay , list);/*Insert this sink primitive in the linked list pointer to by "list"*/
		      
		    }
		}
	    }
	}
    }
  free(temp_port_name);
  return(list);  
}

pb_list *insert_to_linked_list(t_pb *pb_new , pb_list *list)
{
  pb_list *new_list = (pb_list *)malloc(1 * sizeof(pb_list));
  assert(new_list);
  new_list->pb = pb_new;
  new_list->next = list;
  list = new_list;
  return(list);
}

conn_list *insert_to_linked_list_conn(t_pb *driver_new , t_pb *load_new , t_pb_graph_pin *driver_pin_ , t_pb_graph_pin *load_pin_ , float path_delay , conn_list *list)
{
  conn_list *new_list = (conn_list *)malloc(1 * sizeof(conn_list));
  assert(new_list);
  new_list->driver_pb = driver_new;
  new_list->load_pb = load_new;
  new_list->driver_pin = driver_pin_;
  new_list->load_pin = load_pin_;
  new_list->driver_to_load_delay = path_delay;
  new_list->next = list;
  list = new_list;
  return(list);
}

/*traverse_linked_list_conn can be used to print out the found connections of a primitive to the screen */
void traverse_linked_list_conn(conn_list *list)
{
  conn_list *current;
  
  for(current=list ; current != NULL ; current=current->next)
    {
      printf("    driver=> type: %s , name: %s , output: [%d][%d] , load=> type: %s , name: %s input: [%d][%d]\npath delay: %e\n\n",current->driver_pb->pb_graph_node->pb_type->name , 
	     current->driver_pb->name,
	     current->driver_pin->port->index , current->driver_pin->pin_number ,
	     current->load_pb->pb_graph_node->pb_type->name,
	     current->load_pb->name,
	     current->load_pin->port->index , current->load_pin->pin_number,
	     current->driver_to_load_delay );
    }
}

/*traverse_linked_list can be used to print out the primitives of a clb to the screen */
void traverse_linked_list(pb_list *list)
{
  pb_list *current;
  
  for(current=list ; current!=NULL ; current=current->next)
    {
      printf("type: %s , name: %s \n", logical_block[current->pb->logical_block].model->name , current->pb->name);
    }
}

pb_list *free_linked_list(pb_list *list)
{
  pb_list *current;
  pb_list *temp_free;
  
  current=list;
  while(current!=NULL)
    {
      temp_free=current;
      current=current->next;
      free(temp_free);
    }
  list = NULL;
  return(list);
}

conn_list *free_linked_list_conn(conn_list *list)
{
  conn_list *current;
  conn_list *temp_free;
  
  current=list;
  while(current!=NULL)
    {
      temp_free=current;
      current=current->next;
      free(temp_free);
    }
  list = NULL;
  return(list);
}

