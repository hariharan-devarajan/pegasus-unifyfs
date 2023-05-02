#!/bin/bash

python workflow.py --bin /usr/WS2/iopp/software/pegasus-unifyfs/build/apps/posix --data /p/gpfs1/iopp/temp/unifyfs/data --pmc

pegasus-plan --dir work --relative-dir run --dax workflow.yml --output-site local --cluster whole
