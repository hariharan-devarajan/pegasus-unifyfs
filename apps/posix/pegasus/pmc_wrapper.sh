#!/bin/bash
LAUNCH_DIR=`pwd`
echo "Job Launched in directory $LAUNCH_DIR"
pushd $PEGASUS_SCRATCH_DIR
cp $_CONDOR_SCRATCH_DIR/*.in .
PEGASUS_DIR=`which pegasus-mpi-cluster`
if [[ -z "${LD_PRELOAD2}" ]]; then
  echo "Running baseline version"
  echo "jsrun -a 40 -c 40 -r 1 ${PEGASUS_DIR} -v $@"
  jsrun -a 40 -c 40 -r 1 ${PEGASUS_DIR} -v $@
else
  echo "Running unifyfs version"
  export UNIFYFS_LOG_VERBOSITY=5
  export UNIFYFS_MOUNTPOINT=/unifyfs
  export UNIFYFS_LOG_DIR=~/unifyfs/logs/
  export UNIFYFS_LOGIO_SPILL_DIR=/p/gpfs1/iopp/temp/unifyfs/spill
  export UNIFYFS_SERVER_CORES=8
  PROJECT_DIR=/usr/WS2/iopp/software/pegasus-unifyfs
  UNIFYFS_ROOT_DIR=${PROJECT_DIR}/dependency/.spack-env/view
  mkdir -p /p/gpfs1/iopp/temp/unifyfs/share-dir
  echo "${UNIFYFS_ROOT_DIR}/bin/unifyfs start --share-dir=/p/gpfs1/iopp/temp/unifyfs/share-dir -d"
  ${UNIFYFS_ROOT_DIR}/bin/unifyfs start --share-dir=/p/gpfs1/iopp/temp/unifyfs/share-dir -d
  echo "jsrun -a 32 -c 32 -r 1 ${PEGASUS_DIR} -v $@"
  jsrun -a 32 -c 32 -r 1 ${PEGASUS_DIR} -v $@
  ${UNIFYFS_ROOT_DIR}/bin/unifyfs terminate
fi

