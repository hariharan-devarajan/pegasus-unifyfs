#!/usr/bin/env python3
import os
import logging
from pathlib import Path
from argparse import ArgumentParser
import shutil
from pathlib import Path
from typing import Optional

logging.basicConfig(level=logging.DEBUG)

# --- Import Pegasus API ------------------------------------------------------
from Pegasus.api import *


class MimirWorkflow:
    wf = None
    sc = None
    tc = None
    rc = None
    props = None

    dagfile = None
    wf_name = None
    wf_dir = None
    pmc = None
    intercept = None

    bin = None
    data = None
    jobs = None

    # --- Init ----------------------------------------------------------------
    def __init__(self,
                 bin,
                 data,
                 jobs,
                 pmc,
                 intercept):
        self.bin = bin
        self.jobs = jobs
        self.data = data
        self.dagfile = "workflow.yml"
        self.wf_name = "pegasus_io_test"
        self.wf_dir = str(Path(__file__).parent.resolve())
        self.src_path = self.wf_dir
        self.pmc = pmc
        self.intercept = intercept

    # --- Write files in directory --------------------------------------------
    def write(self):
        if not self.sc is None:
            self.sc.write()
        self.props.write()
        self.tc.write()
        self.rc.write()
        self.wf.write()

    # --- Configuration (Pegasus Properties) ----------------------------------
    def create_pegasus_properties(self):
        self.props = Properties()
        self.props["pegasus.register"] = "false"
        self.props["pegasus.integrity.checking"] = "none"
        if self.pmc:
            self.props["pegasus.job.aggregator"] = "mpiexec"
            self.props["pegasus.data.configuration"] = "sharedfs"
        return

    # --- Site Catalog --------------------------------------------------------
    def create_sites_catalog(self, exec_site_name="condorpool"):
        self.sc = SiteCatalog()

        shared_scratch_dir = os.path.join(self.wf_dir, "scratch")
        local_storage_dir = os.path.join(self.wf_dir, "output")

        local = Site("local").add_directories(
            Directory(Directory.SHARED_SCRATCH, shared_scratch_dir).add_file_servers(
                FileServer("file://" + shared_scratch_dir, Operation.ALL)
            ),
            Directory(Directory.LOCAL_STORAGE, local_storage_dir).add_file_servers(
                FileServer("file://" + local_storage_dir, Operation.ALL)
            ),
        )

        executable_path = Path(shutil.which('pegasus-status')).parent.parent.absolute();

        exec_site = (
            Site(exec_site_name)
                .add_pegasus_profile(style="condor")
                .add_condor_profile(universe="vanilla")
                .add_profiles(Namespace.PEGASUS, key="data.configuration", value="sharedfs")
                .add_profiles(Namespace.PEGASUS, key="auxillary.local", value="true")
                .add_profiles(Namespace.ENV, key="PEGASUS_HOME", value=f"{executable_path}")
                .add_directories(
                Directory(Directory.SHARED_SCRATCH, shared_scratch_dir).add_file_servers(
                    FileServer("file://" + shared_scratch_dir, Operation.ALL)
                ),
                Directory(Directory.LOCAL_STORAGE, local_storage_dir).add_file_servers(
                    FileServer("file://" + local_storage_dir, Operation.ALL)
                ),
            )
        )

        self.sc.add_sites(local, exec_site)

    # --- Transformation Catalog (Executables and Containers) -----------------
    def create_transformation_catalog(self, exec_site_name="condorpool"):
        self.tc = TransformationCatalog()

        executable_path = Path(shutil.which('pegasus-status')).parent.parent.absolute();
        path = self.bin
        ld_preload = ""
        if self.intercept:
            if "UNIFYFS_LIB_PATH" not in os.environ:
                raise Exception('UNIFYFS_LIB_PATH not set. Needs to point to libunifyfs_gotcha.so.')
            ld_preload = os.environ["UNIFYFS_LIB_PATH"]
            #path = "/unifyfs"
        filename = os.path.join(path, "pegasus")
        raw = Transformation(
            "pegasus_raw", site=exec_site_name, pfn=filename, is_stageable=False,
        )
        write = Transformation(
            "pegasus_write", site=exec_site_name, pfn=filename, is_stageable=False,
        )
        read = Transformation(
            "pegasus_read", site=exec_site_name, pfn=filename, is_stageable=False,
        )
        if self.pmc:
            pmc_wrapper_pfn = self.src_path + '/pmc_wrapper.sh'
            path = os.environ["PATH"] + ":."
            pmc = (
                Transformation("mpiexec", namespace="pegasus", site=exec_site_name, pfn=pmc_wrapper_pfn,
                               is_stageable=False)
                    .add_profiles(Namespace.PEGASUS, key="job.aggregator", value="mpiexec")
                    .add_profiles(Namespace.ENV, key="PATH", value=path)
                    .add_profiles(Namespace.ENV, key="PEGASUS_HOME", value=f"{executable_path}")
                    .add_profiles(Namespace.ENV, key="LD_PRELOAD2", value=f"{ld_preload}")
                    .add_profiles(Namespace.CONDOR, key="getenv", value="*")
            )
            self.tc.add_transformations(pmc)
        self.tc.add_transformations(raw, write, read)
        # self.tc.add_transformations(write, read)

    # --- Replica Catalog ------------------------------------------------------
    def create_replica_catalog(self):
        self.rc = ReplicaCatalog()

    # --- Create Workflow -----------------------------------------------------
    def create_workflow(self):
        self.wf = Workflow(self.wf_name, infer_dependencies=True)
        if self.intercept:
            path = "/unifyfs"
        else:
            path = self.data
        # RAW usecase no job dependency.
        raws = []
        for i in range(self.jobs):
            raw = (
                Job("pegasus_raw")
                    .add_args("--durations", "yes", "--reporter", "compact",
                              "--pfs", self.data, "--filename", f"raw_{i}.dat", "[operation=raw]")
            )
            raws.append(raw)
        # Read job depends on Write job.
        reads = []
        writes = []
        for i in range(self.jobs):
            filename = f"test_{i}.dat"
            file_data = File(filename)
            write = (
                Job("pegasus_write")
                    .add_args("--durations", "yes", "--reporter", "compact",
                              "--pfs", path, "--filename", filename, "[operation=write]")
                    .add_outputs(file_data, stage_out=False, register_replica=False)
            )
            read = (
                Job("pegasus_read")
                    .add_args("--durations", "yes", "--reporter", "compact",
                              "--pfs", path, "--filename", filename, "[operation=read]")
                    .add_inputs(file_data)
            )
            writes.append(write)
            reads.append(read)

        self.wf.add_jobs(raw, *writes, *reads)
        # self.wf.add_jobs(write, read)


if __name__ == "__main__":
    parser = ArgumentParser(description="Pegasus Write + Read Workflow")

    parser.add_argument(
        "--data",
        metavar="STR",
        type=str,
        default="${PWD}",
        help="Path for data",
    )
    parser.add_argument(
        "--bin",
        metavar="STR",
        type=str,
        default="${PWD}",
        help="Binary directory for tests",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=8,
        help="# of jobs per type of test",
    )
    parser.add_argument('--intercept', action='store_true', default=False, dest='intercept', help='Intercept using Unifyfs')
    parser.add_argument('--pmc', action='store_true', dest='pmc', help='Use PMC')

    args = parser.parse_args()
    workflow = MimirWorkflow(args.bin, args.data, args.jobs, args.pmc, args.intercept)

    print("Creating execution sites...")
    workflow.create_sites_catalog()

    print("Creating workflow properties...")
    workflow.create_pegasus_properties()

    print("Creating transformation catalog...")
    workflow.create_transformation_catalog()

    print("Creating replica catalog...")
    workflow.create_replica_catalog()

    print("Creating split workflow dag...")
    workflow.create_workflow()

    workflow.write()
