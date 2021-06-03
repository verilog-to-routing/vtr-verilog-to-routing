#! /bin/bash

#SBATCH --nodes=1                                       # Number of nodes in our pool
#SBATCH --ntasks=80                                     # Number of parallel tasks
#SBATCH --ntasks-per-node=40                            # maximum number of tasks simultaneously running on a node
#SBATCH --job-name=vtr_test                             # Job name
#SBATCH --mail-type=FAIL,END                            # When to send an email
#SBATCH --mail-user=XXXX@mail.utoronto.ca               # The user's email
#SBATCH --output=parallel_%j.log                        # Determine the output log file (%j) is the jobid
#SBATCH --error=error_%j.log                            # Determine the error log file
#SBATCH --time=10:00:00                                 # The job time limit in hh:mm:ss

#Preload the required environment modules
module load gcc/9.2.0
module load python/3.8.5

#Choose the run directory 
path_to_task="../../tasks/regression_tests/vtr_reg_nightly/vtr_reg_qor_chain_large"
dir="run001"


for script in ${path_to_task}/${dir}/*/*/common*/vtr_flow.sh
do
        echo $script
        $script &
done
wait
