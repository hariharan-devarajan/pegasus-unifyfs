#!/bin/bash

export UNIFYFS_LIB_PATH=/usr/WS2/iopp/software/pegasus-unifyfs/dependency/.spack-env/view/lib/libunifyfs_preload_gotcha.so
python workflow.py --bin /usr/WS2/iopp/software/pegasus-unifyfs/build/apps/posix --data /p/gpfs1/iopp/temp/unifyfs/data --jobs 1 --intercept --pmc

pegasus-plan --dir work --relative-dir run --dax workflow.yml --output-site local --cluster whole
