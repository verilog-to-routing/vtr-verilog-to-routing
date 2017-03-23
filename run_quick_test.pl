#!/usr/bin/perl
#------------------------------------------------------------------------#
#  Run an experiment on the batch system                                 #
#   - runs an experiment script                                          #
#     creates seperate directory for each circuit                        #
#   - uses a configuration file for experiment setup                     #
#                                                                        #
#  Usage:                                                                #
#  <While in the experiment directory with directory base>               #
#  run_experiment.pl [local|condor]                                      #
#                                                                        #
#     local|condor - an optional string that is either local to run      #
#                    locally or condor to run on condor batch system,    #
#					 defaults to condor                                  #
#    Options: -machines [fast|slow|both]                                 #
#------------------------------------------------------------------------#

use strict;
use Cwd;

system("./run_reg_test.pl vtr_reg_basic");

