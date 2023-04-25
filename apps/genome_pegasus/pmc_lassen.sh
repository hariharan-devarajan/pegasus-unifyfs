#!/bin/bash

set -e
#source /usr/workspace/iopp/install_scripts/bin/iopp-init
#source /usr/workspace/iopp/install_scripts/bin/spack-init
unset RECORDER_NO_MPI
unset LD_PRELOAD
#spack env activate montage-pegasus
LAUNCH_DIR=`pwd`
echo "Job Launched in directory $LAUNCH_DIR"
pushd $PEGASUS_SCRATCH_DIR
cp $_CONDOR_SCRATCH_DIR/*.in .
echo "/usr/tcetmp/bin/jsrun /usr/WS2/iopp/software/pegasus/install/bin/pegasus-mpi-cluster -v $@"
recorder_lib_path=/usr/workspace/iopp/software/Recorder/install/lib/librecorder.so
export RECORDER_TRACES_DIR=/p/gpfs1/iopp/recorder_logs/genome_peagsus_${LSB_JOBID}
echo "trace in dir $RECORDER_TRACES_DIR"
/usr/tcetmp/bin/jsrun --env LD_PRELOAD=$recorder_lib_path --env RECORDER_WITH_NON_MPI=1  /usr/WS2/iopp/software/pegasus/install/bin/pegasus-mpi-cluster -v "$@"
#/usr/tcetmp/bin/jsrun /usr/WS2/iopp/software/pegasus/install/bin/pegasus-mpi-cluster -v "$@"
