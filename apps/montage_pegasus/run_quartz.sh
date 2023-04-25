#!/bin/bash
### Slurm syntax
#SBATCH -N 1            # number of nodes
#SBATCH -t 120             #walltime in minutes
#SBATCH -A asccasc           #account
#SBATCH --job-name=montage-pegasus   #name of job
#SBATCH -p pdebug            #queue to use

CPWD=$PWD
source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init

module load mpifileutils/0.11
spack env activate montage-pegasus-x86


source /usr/WS2/iopp/software/htcondor-V9.0.6/install/bin/condor.sh

/usr/WS2/iopp/software/htcondor-V9.0.6/install/sbin/condor_master

sleep 10

pushd /usr/workspace/iopp/applications/montage-workflow-v3

JOB_DIR=$pfs/job/$USER

rm -r $JOB_DIR

mkdir -p $JOB_DIR

cp $CPWD/sites.quartz.yml $JOB_DIR/sites.yml
cp $CPWD/pegasus.properties $JOB_DIR/ 

pushd $JOB_DIR

rm -rf $JOB_DIR/data
/usr/workspace/iopp/applications/montage-workflow-v3/montage-workflow.py --center "56.7 24.00" --degrees 2.0 --band dss:DSS2B:blue --band dss:DSS2R:green --band dss:DSS2IR:red



#pegasus-plan --dir work --dax data/montage-workflow.yml --output-site local --cluster horizontal --relative-dir run_dir
pegasus-plan --dir work --dax data/montage-workflow.yml --output-site slurm -s slurm --cluster horizontal --relative-dir run_dir -s slurm

pushd $JOB_DIR/work/run_dir

pegasus-run $PWD

pegasus-status --watch $PWD

popd
popd
popd
