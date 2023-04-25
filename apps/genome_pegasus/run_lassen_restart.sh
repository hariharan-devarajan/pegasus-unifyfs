#!/bin/bash
### LSF syntax
#BSUB -cwd /usr/workspace/iopp/software/iopp/apps/genome_pegasus 
#BSUB -nnodes 4            #number of nodes
#BSUB -W 02:00             #walltime in minutes
#BSUB -G asccasc           #account
#BSUB -J genome-pegasus   #name of job
#BSUB -q pdebug            #queue to use

CPWD=$PWD
source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init

spack env activate -p genome-pegasus
export PATH=/usr/WS2/iopp/software/pegasus/install/bin:$PATH
export PEGASUS_HOME=/usr/WS2/iopp/software/pegasus/install
source /usr/WS2/iopp/software/htcondor-V9.0.6/install/bin/condor.sh

/usr/WS2/iopp/software/htcondor-V9.0.6/install/sbin/condor_master

sleep 10

JOB_DIR=/p/gpfs1/iopp/temp/1000-genome-$USER
#rm -r $JOB_DIR
#mkdir -p $JOB_DIR
#cp -r /usr/workspace/iopp/applications/1000genome-workflow/* $JOB_DIR/

#pushd $JOB_DIR

#cp $CPWD/sites.yml sites.local.yml 


#./daxgen.py -S sites.local.yml -e local -n run_dir
pegasus-remove /p/gpfs1/iopp/temp/1000-genome-haridev/run_dir

sleep 10
pushd $JOB_DIR/run_dir

pegasus-run $PWD
sleep 7000
pegasus-status -l $PWD
pegasus-remove $PWD

#popd
popd
