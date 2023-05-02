#!/bin/bash
### LSF syntax
#BSUB -cwd /usr/WS2/iopp/software/pegasus-unifyfs/scripts/posix
#BSUB -nnodes 1         #number of nodes
#BSUB -W 02:00             #walltime in minutes
#BSUB -G asccasc           #account
#BSUB -J posix-pegasus   #name of job
#BSUB -q pbatch            #queue to use

PROJECT_DIR=/usr/WS2/iopp/software/pegasus-unifyfs
PEGASUS_PATH=/usr/WS2/iopp/software/pegasus/install
CONDOR_PATH=/usr/WS2/iopp/software/htcondor-V9.0.6/install
BASE_PEGASUS_PATH=/p/gpfs1/iopp/temp/unifyfs/pegasus/posix/${USER}

source /usr/workspace/iopp/install_scripts/bin/iopp-init
source /usr/workspace/iopp/install_scripts/bin/spack-init


spack env activate -p ${PROJECT_DIR}/dependency
source ${PROJECT_DIR}/dependency/python-env/bin/activate

source ${CONDOR_PATH}/bin/condor.sh

echo "Loading environment - Done"
sleep 5

export PATH=${PROJECT_DIR}/depedency/.spack-env/view/bin:${PEGASUS_PATH}/bin:$PATH
export PEGASUS_HOME=${PEGASUS_PATH}
${CONDOR_PATH}/sbin/condor_master


echo "Loading condor - Done"


JOB_DIR=${BASE_PEGASUS_PATH}/app
SCRATCH_DIR=${BASE_PEGASUS_PATH}/scratch

echo "Cleaning folders"
mv ${BASE_PEGASUS_PATH} ${BASE_PEGASUS_PATH}.tmp
rm -rf ${BASE_PEGASUS_PATH}.tmp &

sleep 10
echo "Preparing workflow folder - Start"
mkdir -p $JOB_DIR
mkdir -p $SCRATCH_DIR
cp -r ${PROJECT_DIR}/apps/posix/pegasus/pegasus/* $JOB_DIR
cp ${PROJECT_DIR}/apps/posix/pegasus/pmc_wrapper.sh $JOB_DIR
cp ${PROJECT_DIR}/apps/posix/pegasus/workflow.py $JOB_DIR

pushd $JOB_DIR

echo "Building workflow folder - Done"
chmod +x ./create_workflow.sh
./create_workflow.sh

pushd $JOB_DIR/work/run

echo "Planning of workflow - Done"
pegasus-run $PWD
sleep $((2*3550))
pegasus-status -l $PWD
pegasus-remove $PWD

popd
popd