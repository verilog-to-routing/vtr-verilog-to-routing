submission_template.sh is a template used to submit jobs using SLURM on a large cluster
- This script is tested on computecanada cluster (Niagara) system

To use this script:
- First, generate all the shell scripts to run a task using:
    * run_vtr_task.py -system scripts (Sec. 3.9.3 in the documentation)
    * this will create a new run directory under the specified task (e.g. run001)

- Second, edit the submission_template.sh script to match your run
    * All lines starting with #SBATCH go to SLURM
    * Edit all these line depending on the number of cores you want, number of parallel jobs you want,
      the job name, your email and the job time limit
    * Edit the relative path to your task (path_to_task) and the specific run directory where the 
      generated shell scripts are (dir)

- Third, submit these jobs to SLURM using the following command
    * sbatch submission_template.sh

You can check the state of the submitted job (or all the jobs that you submitted) using
squeue --me

