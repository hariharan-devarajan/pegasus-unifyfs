
#include "catch_config.h"
#include <test_utils.h>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <fcntl.h>
#include <mpi.h>
/**
 * Test data structures
 */
namespace mimir::test {
struct Arguments {
  fs::path pfs = "/home/hariharan/temp/mimir/pfs";
  fs::path shm = "/home/hariharan/temp/mimir/shm";
  std::string filename = "test.dat";
  size_t request_size = 65536;
  size_t iteration = 8;
  bool debug = false;
};
}  // namespace mimir::test

mimir::test::Arguments args;

/**
 * Overridden methods for catch
 */

int init(int* argc, char*** argv) {
  //  fprintf(stdout, "Initializing MPI\n");
  MPI_Init(argc, argv);
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  if (args.debug && my_rank == 0) {
    printf("%d ready for attach\n", comm_size);
    fflush(stdout);
    getchar();
  }
  MPI_Barrier(MPI_COMM_WORLD);
  return 0;
}
int finalize() {
  MPI_Finalize();
  return 0;
}

cl::Parser define_options() {
  return cl::Opt(args.filename, "filename")["-f"]["--filename"](
             "Filename to be use for I/O.") |
         cl::Opt(args.pfs, "pfs")["-p"]["--pfs"](
             "Directory used for performing I/O (default pfs)") |
         cl::Opt(args.shm, "shm")["-s"]["--shm"](
             "Directory used for performing I/O (default shm)") |
         cl::Opt(args.request_size, "request_size")["-r"]["--request_size"](
             "Transfer size used for performing I/O") |
         cl::Opt(args.iteration,
                 "iteration")["-i"]["--iteration"]("Number of Iterations");
}

/**
 * utility methods
 */
inline std::string GetFilenameFromFD(int fd) {
  const int kMaxSize = 0xFFF;
  char proclnk[kMaxSize];
  char filename[kMaxSize];
  snprintf(proclnk, kMaxSize, "/proc/self/fd/%d", fd);
  size_t r = readlink(proclnk, filename, kMaxSize);
  filename[r] = '\0';
  return filename;
}

/**
 * Test cases
 */

TEST_CASE("Write",
          "[operation=write]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization;

  initialization.resumeTime();
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  args.filename = args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size);
  printf("filename %s\n", args.filename.c_str());
  fs::path filepath = args.pfs / args.filename;
  fs::create_directories(args.pfs);
  /** Clean existing file**/
  if (fs::exists(filepath)) fs::remove(filepath);
  /** Prepare data **/
  auto write_data = std::vector<char>(args.request_size, 'w');
  initialization.pauseTime();

  /** Main I/O **/
  metadata.resumeTime();
  int fd =
      open(filepath.c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  metadata.pauseTime();
  REQUIRE(fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_written = write(fd, write_data.data(), args.request_size);
    int fsync_status = fsync(fd);
    io.pauseTime();
    REQUIRE(bytes_written == args.request_size);
  }
  finalization.resumeTime();
  auto new_file = GetFilenameFromFD(fd);
  finalization.pauseTime();
  metadata.resumeTime();
  int close_status = close(fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  REQUIRE(fs::file_size(new_file) == args.request_size * args.iteration);

  finalization.resumeTime();
  printf("I/O performed on file %s\n", new_file.c_str());
  finalization.pauseTime();
  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, and finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());
}

TEST_CASE("Read",
          "[operation=read]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization;

  initialization.resumeTime();
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  args.filename = args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size);
  fs::path filepath = args.pfs / args.filename;
  /** Prepare data **/
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();

  MPI_Barrier(MPI_COMM_WORLD);
  /** Main I/O **/
  metadata.resumeTime();
  int fd = open(filepath.c_str(), O_RDONLY);
  metadata.pauseTime();
  REQUIRE(fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_read = read(fd, read_data.data(), args.request_size);
    int fsync_status = fsync(fd);
    io.pauseTime();
    REQUIRE(bytes_read == args.request_size);
  }

  finalization.resumeTime();
  auto new_file = GetFilenameFromFD(fd);
  finalization.pauseTime();

  metadata.resumeTime();
  int close_status = close(fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  REQUIRE(fs::file_size(new_file) == args.request_size * args.iteration);

  finalization.resumeTime();
  printf("I/O performed on file %s\n", new_file.c_str());
  finalization.pauseTime();
  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, and finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());

  if (fs::exists(filepath)) fs::remove(filepath);
  filepath = args.shm / args.filename;
  if (fs::exists(filepath)) fs::remove(filepath);
}

TEST_CASE("ReadAfterWrite",
          "[operation=raw]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization;

  initialization.resumeTime();
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  args.filename = args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size);
  fs::path filepath = args.pfs / args.filename;
  fs::create_directories(args.pfs);
  /** Clean existing file**/
  if (fs::exists(filepath)) fs::remove(filepath);
  /** Prepare data **/
  auto write_data = std::vector<char>(args.request_size, 'w');
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();

  using namespace mimir;



  /** Write I/O **/
  metadata.resumeTime();
  int write_fd =
      open(filepath.c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  metadata.pauseTime();
  REQUIRE(write_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_written =
        write(write_fd, write_data.data(), args.request_size);
    int fsync_status = fsync(write_fd);
    io.pauseTime();
    REQUIRE(bytes_written == args.request_size);
  }
  finalization.resumeTime();
  auto new_file_write = GetFilenameFromFD(write_fd);
  finalization.pauseTime();
  metadata.resumeTime();
  int close_status_write = close(write_fd);
  metadata.pauseTime();
  REQUIRE(close_status_write == 0);
  REQUIRE(fs::file_size(new_file_write) == args.request_size * args.iteration);

  finalization.resumeTime();
  printf("Write I/O performed on file %s\n", new_file_write.c_str());
  finalization.pauseTime();

  /** Read I/O **/
  metadata.resumeTime();
  int read_fd = open(filepath.c_str(), O_RDONLY);
  metadata.pauseTime();
  REQUIRE(read_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_read = read(read_fd, read_data.data(), args.request_size);
    int fsync_status = fsync(read_fd);
    io.pauseTime();
    REQUIRE(bytes_read == args.request_size);
  }

  finalization.resumeTime();
  auto new_file_read = GetFilenameFromFD(read_fd);
  finalization.pauseTime();

  metadata.resumeTime();
  int close_status = close(read_fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  finalization.resumeTime();
  printf("Read I/O performed on file %s\n", new_file_read.c_str());
  finalization.pauseTime();
  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, and finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());
  if (fs::exists(filepath)) fs::remove(filepath);
  filepath = args.shm / args.filename;
  if (fs::exists(filepath)) fs::remove(filepath);
}

TEST_CASE("ReadAfterWriteShared",
          "[operation=raw_shared]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization;

  initialization.resumeTime();

  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  auto my_io_filename =
      args.shm / (args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size));

  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  my_io_filename = args.pfs / (args.filename + "." + std::to_string(my_rank) +
                               "." + std::to_string(comm_size));
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  MPI_Barrier(MPI_COMM_WORLD);
  using namespace mimir;


  fs::create_directories(args.pfs);
  /** Clean existing file**/
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  /** Prepare data **/
  auto write_data = std::vector<char>(args.request_size, 'w');
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();

  /** Write I/O **/
  metadata.resumeTime();
  int write_fd = open(my_io_filename.c_str(), O_WRONLY | O_CREAT,
                      S_IRWXU | S_IRWXG | S_IRWXO);
  metadata.pauseTime();
  REQUIRE(write_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_written =
        write(write_fd, write_data.data(), args.request_size);
    int fsync_status = fsync(write_fd);
    io.pauseTime();
  }
  metadata.resumeTime();
  int close_status_write = close(write_fd);
  metadata.pauseTime();
  REQUIRE(close_status_write == 0);

  finalization.resumeTime();
  printf("Write I/O performed on file %s\n", my_io_filename.c_str());
  finalization.pauseTime();

  /* Read I/O */
  metadata.resumeTime();
  int read_fd = open(my_io_filename.c_str(), O_RDONLY);
  metadata.pauseTime();
  REQUIRE(read_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_read = read(read_fd, read_data.data(), args.request_size);
    int fsync_status = fsync(read_fd);
    io.pauseTime();
    REQUIRE(bytes_read == args.request_size);
  }

  metadata.resumeTime();
  int close_status = close(read_fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, and finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  my_io_filename = args.shm / (args.filename + "." + std::to_string(my_rank) +
                               "." + std::to_string(comm_size));
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
}

TEST_CASE("OnlyReadInputFiles",
          "[operation=input]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization, compute;

  initialization.resumeTime();
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  auto my_io_filename =
      args.pfs / (args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size));
  fs::create_directories(args.pfs);
  if (my_rank == 0) {
    for (int i = 0; i < comm_size; ++i) {
      auto filename = args.pfs / (args.filename + "." + std::to_string(i) +
                                  "." + std::to_string(comm_size));
      std::string cmd = "{ tr -dc '[:alnum:]' < /dev/urandom | head -c " +
                        std::to_string(args.request_size * args.iteration) +
                        "; } > " + filename.c_str() + " ";
      int status = system(cmd.c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);


  /** Prepare data **/
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();
  /* Computation */
  compute.resumeTime();
  sleep(5);
  compute.pauseTime();

  /* Read I/O */
  metadata.resumeTime();
  int read_fd = open(my_io_filename.c_str(), O_RDONLY);
  metadata.pauseTime();
  REQUIRE(read_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_read = read(read_fd, read_data.data(), args.request_size);
    int fsync_status = fsync(read_fd);
    io.pauseTime();
    REQUIRE(bytes_read == args.request_size);
  }

  metadata.resumeTime();
  int close_status = close(read_fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);


  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, compute %f, and "
          "finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), compute.getElapsedTime(),
          finalization.getElapsedTime());
}

TEST_CASE("ReadOnly",
          "[operation=read_only]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization, compute;

  initialization.resumeTime();
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  auto my_io_filename =
      args.pfs / (args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size));
  fs::create_directories(args.pfs);
  if (my_rank == 0) {
    for (int i = 0; i < comm_size; ++i) {
      auto filename = args.pfs / (args.filename + "." + std::to_string(i) +
                                  "." + std::to_string(comm_size));
      std::string cmd = "{ tr -dc '[:alnum:]' < /dev/urandom | head -c " +
                        std::to_string(args.request_size * args.iteration) +
                        "; } > " + filename.c_str() + " ";
      int status = system(cmd.c_str());
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);


  /** Prepare data **/
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();
  /* Computation */
  compute.resumeTime();
  sleep(5);
  compute.pauseTime();

  /* Read I/O */
  metadata.resumeTime();
  int read_fd = open(my_io_filename.c_str(), O_RDONLY);
  metadata.pauseTime();
  REQUIRE(read_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_read = read(read_fd, read_data.data(), args.request_size);
    int fsync_status = fsync(read_fd);
    io.pauseTime();
    REQUIRE(bytes_read == args.request_size);
  }

  metadata.resumeTime();
  int close_status = close(read_fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);
  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, compute %f, and "
          "finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), compute.getElapsedTime(),
          finalization.getElapsedTime());
}

TEST_CASE("PriorityWrite",
          "[operation=priority_write]"
          "[request_size=" +
              std::to_string(args.request_size) +
              "]"
              "[iteration=" +
              std::to_string(args.iteration) + "]") {
  Timer initialization, metadata, io, finalization;

  initialization.resumeTime();

  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  auto my_io_filename =
      args.shm / (args.filename + "." + std::to_string(my_rank) + "." +
                  std::to_string(comm_size));

  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  my_io_filename = args.pfs / (args.filename + "." + std::to_string(my_rank) +
                               "." + std::to_string(comm_size));
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  MPI_Barrier(MPI_COMM_WORLD);

  fs::create_directories(args.pfs);
  /** Clean existing file**/
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  /** Prepare data **/
  auto write_data = std::vector<char>(args.request_size, 'w');
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();

  /** Write I/O **/
  metadata.resumeTime();
  int write_fd = open(my_io_filename.c_str(), O_WRONLY | O_CREAT,
                      S_IRWXU | S_IRWXG | S_IRWXO);
  metadata.pauseTime();
  REQUIRE(write_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_written =
        write(write_fd, write_data.data(), args.request_size);
    int fsync_status = fsync(write_fd);
    io.pauseTime();
  }
  metadata.resumeTime();
  int close_status_write = close(write_fd);
  metadata.pauseTime();
  REQUIRE(close_status_write == 0);

  finalization.resumeTime();
  printf("Write I/O performed on file %s\n", my_io_filename.c_str());
  finalization.pauseTime();

  /* Read I/O */
  metadata.resumeTime();
  int read_fd = open(my_io_filename.c_str(), O_RDONLY);
  metadata.pauseTime();
  REQUIRE(read_fd != -1);

  for (size_t i = 0; i < args.iteration; ++i) {
    io.resumeTime();
    ssize_t bytes_read = read(read_fd, read_data.data(), args.request_size);
    int fsync_status = fsync(read_fd);
    io.pauseTime();
    REQUIRE(bytes_read == args.request_size);
  }

  metadata.resumeTime();
  int close_status = close(read_fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  fprintf(stdout,
          "Timing rank %d: init %f, metadata %f, io %f, and finalize %f.\n",
          my_rank, initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
  my_io_filename = args.shm / (args.filename + "." + std::to_string(my_rank) +
                               "." + std::to_string(comm_size));
  if (fs::exists(my_io_filename)) fs::remove(my_io_filename);
}