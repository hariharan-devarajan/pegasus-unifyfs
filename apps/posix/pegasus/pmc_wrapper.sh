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
  PROJECT_DIR=/usr/WS2/iopp/software/pegasus-unifyfs
  UNIFYFS_ROOT_DIR=${PROJECT_DIR}/dependency/.spack-env/view
  mkdir -p /p/gpfs1/iopp/temp/unifyfs/share-dir
  UNIFYFS_LOG_DIR=~/unifyfs/logs UNIFYFS_SERVER_CORES=8 ${UNIFYFS_ROOT_DIR}/bin/unifyfs start --share-dir=/p/gpfs1/iopp/temp/unifyfs/share-dir -d
  echo "jsrun -a 40 -c 40 -r 1 -env LD_PRELOAD $LD_PRELOAD2 ${PEGASUS_DIR} -v $@"
  jsrun -a 40 -c 40 -r 1 -env LD_PRELOAD $LD_PRELOAD2 ${PEGASUS_DIR} -v $@
fi

