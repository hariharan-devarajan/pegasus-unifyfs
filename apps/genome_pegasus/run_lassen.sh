#!/bin/bash
### LSF syntax
#BSUB -cwd /usr/workspace/iopp/software/iopp/apps/genome_pegasus 
#BSUB -nnodes 32         #number of nodes
#BSUB -W 03:00             #walltime in minutes
#BSUB -G asccasc           #account
#BSUB -J genome-pegasus   #name of job
#BSUB -q pbatch            #queue to use

CPWD=$PWD
source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init
module load mpifileutils/0.11
spack env activate -p genome-pegasus


source /usr/WS2/iopp/software/htcondor-V9.0.6/install/bin/condor.sh

echo "Loading environment - Done"
sleep 5

export PATH=/usr/WS2/iopp/software/spack/var/spack/environments/genome-pegasus/.spack-env/view/bin:/usr/WS2/iopp/software/pegasus/install/bin:$PATH
export PEGASUS_HOME=/usr/WS2/iopp/software/pegasus/install
/usr/WS2/iopp/software/htcondor-V9.0.6/install/sbin/condor_master


echo "Loading condor - Done"

JOB_DIR=/p/gpfs1/iopp/temp/1000-genome-$USER/app
SCRATCH_DIR=/p/gpfs1/iopp/temp/1000-genome-$USER/scratch

echo "Cleaning folders"
mv /p/gpfs1/iopp/temp/1000-genome-$USER /p/gpfs1/iopp/temp/.tmp
rm -rf /p/gpfs1/iopp/temp/.tmp &

sleep 10
echo "Preparing workflow folder - Start"
mkdir -p $JOB_DIR
mkdir -p $SCRATCH_DIR
cp -r /usr/workspace/iopp/applications/1000genome-workflow/* $JOB_DIR

pushd $JOB_DIR

cp $CPWD/sites.yml.lassen sites.lassen.yml 
cp $CPWD/daxgen_lassen.py daxgen_lassen.py

echo "Building workflow folder - Done"
./daxgen_lassen.py -S sites.lassen.yml -e condorpool -n run_dir --pmc

pushd $JOB_DIR/run_dir

echo "Planning of workflow - Done"
pegasus-run $PWD
sleep $((3*3550))
pegasus-status -l $PWD
pegasus-remove $PWD

popd
popd
