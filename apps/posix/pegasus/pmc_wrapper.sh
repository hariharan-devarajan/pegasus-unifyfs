#!/bin/bash
LAUNCH_DIR=`pwd`
echo "Job Launched in directory $LAUNCH_DIR"
pushd $PEGASUS_SCRATCH_DIR
cp $_CONDOR_SCRATCH_DIR/*.in .
PEGASUS_DIR=`which pegasus-mpi-cluster`
if [[ -z "${DEPLOY_ENV}" ]]; then
  echo "jsrun -a 40 -c 40 -r 1 ${PEGASUS_DIR} -v $@"
  jsrun -a 40 -c 40 -r 1 ${PEGASUS_DIR} -v $@
else
  echo "jsrun -a 40 -c 40 -r 1 -env LD_PRELOAD $LD_PRELOAD2 ${PEGASUS_DIR} -v $@"
  jsrun -a 40 -c 40 -r 1 -env LD_PRELOAD $LD_PRELOAD2 ${PEGASUS_DIR} -v $@
fi

