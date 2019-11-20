#include <stdio.h>
#include <math.h>
#include "pr.h"
#include "ext.h"
#include "util.h"

void print_wirelen_prob_dist (void) {
 
/* Prints out the probability distribution of the wirelength / number   *
 * input pins on a net -- i.e. simulates 2-point net length probability *
 * distribution.                                                        */
 
 float *prob_dist;
 float norm_fac, two_point_length;
 int inet, bends, length, index;
 float av_length;
 void get_num_bends_and_length (int inet, int *bends, int *length);
 
 prob_dist = (float *) my_calloc (nx + ny + 3, sizeof (float));
 norm_fac = 0.;
 
 for (inet=0;inet<num_nets;inet++) {
    if (is_global[inet] == 0) {
       get_num_bends_and_length (inet, &bends, &length);

/*  Assign probability to two integer lengths proportionately -- i.e.  *
 *  if two_point_length = 1.9, add 0.9 of the pins to prob_dist[2] and *
 *  only 0.1 to prob_dist[1].                                          */

       two_point_length = (float) length / (float) (net[inet].num_pins - 1);
       index = (int) two_point_length;
       prob_dist[index] += (net[inet].num_pins - 1.) * (1 - two_point_length
                                 + index); 

       index++;
       prob_dist[index] += (net[inet].num_pins - 1.) * (1 - index + 
           two_point_length);
       
       norm_fac += net[inet].num_pins - 1.;
    }
 }
 
/* Normalize so total probability is 1 and print out. */
 
 printf ("\nProbability distribution of 2-pin net lengths:\n\n");
 printf ("Length    p(Lenth)\n");
 
 av_length = 0;

 for (index=0;index<nx+ny+3;index++) {
    prob_dist[index] /= norm_fac;
    printf("%6d  %10.6f\n", index, prob_dist[index]);
    av_length += prob_dist[index] * index;
 }

 printf("\nExpected value of 2-pin net length (R) is: %f\n",
       av_length);
 
 free ((void *) prob_dist);
}


void print_lambda (void) {

/* Finds the average number of input pins used per clb.  Does not   *
 * count inputs which are hooked to global nets (i.e. the clock     *
 * when it is marked global).                                       */
 
 int bnum, ipin;
 int num_inputs_used = 0;
 int class, inet;
 float lambda;
 
 for (bnum=0;bnum<num_blocks;bnum++) {
    if (block[bnum].type == CLB) {
       for (ipin=0;ipin<clb_size;ipin++) {
          class = clb_pin_class[ipin];
             if (class_inf[class].type == RECEIVER) {
                inet = block[bnum].nets[ipin];
                if (inet != OPEN)               /* Pin is connected? */
                   if (is_global[inet] == 0)    /* Not a global clock */
                      num_inputs_used++;
             }
       }
    }
 }

 lambda = (float) num_inputs_used / (float) num_clbs;
 printf("Average lambda (input pins used per clb) is: %f\n", lambda);
}
