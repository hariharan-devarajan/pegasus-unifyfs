
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
  fs::path pfs = "/home/haridev/pfs";
  std::string filename = "test.dat";
  size_t request_size = 65536;
  size_t iteration = 64;
};
}  // namespace mimir::test

mimir::test::Arguments args;

/**
 * Overridden methods for catch
 */

int init(int* argc, char*** argv) {
  return 0;
}
int finalize() {
  return 0;
}

cl::Parser define_options() {
  return cl::Opt(args.filename, "filename")["-f"]["--filename"](
             "Filename to be use for I/O.") |
         cl::Opt(args.pfs, "pfs")["-p"]["--pfs"](
             "Directory used for performing I/O (default pfs)") |
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
  fs::path filepath = args.pfs / args.filename;
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
  finalization.pauseTime();
  metadata.resumeTime();
  int close_status = close(fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  //REQUIRE(fs::file_size(new_file) == args.request_size * args.iteration);

  finalization.resumeTime();
  finalization.pauseTime();

  fprintf(stdout, "Timing: init %f, metadata %f, io %f, and finalize %f.\n",
          initialization.getElapsedTime(), metadata.getElapsedTime(),
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
  fs::path filepath = args.pfs / args.filename;
  /** Prepare data **/
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();
  fprintf(stdout, "file to read %s\n", filepath.c_str());

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
  finalization.pauseTime();

  metadata.resumeTime();
  int close_status = close(fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  finalization.resumeTime();
  finalization.pauseTime();

  fprintf(stdout, "Timing: init %f, metadata %f, io %f, and finalize %f.\n",
          initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());
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
  fs::path filepath = args.pfs / args.filename;
  /** Prepare data **/
  auto write_data = std::vector<char>(args.request_size, 'w');
  auto read_data = std::vector<char>(args.request_size, 'r');
  initialization.pauseTime();

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
  finalization.pauseTime();
  metadata.resumeTime();
  int close_status_write = close(write_fd);
  metadata.pauseTime();
  REQUIRE(close_status_write == 0);

  finalization.resumeTime();
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
  finalization.pauseTime();

  metadata.resumeTime();
  int close_status = close(read_fd);
  metadata.pauseTime();
  REQUIRE(close_status == 0);

  finalization.resumeTime();
  finalization.pauseTime();
  fprintf(stdout, "Timing: init %f, metadata %f, io %f, and finalize %f.\n",
          initialization.getElapsedTime(), metadata.getElapsedTime(),
          io.getElapsedTime(), finalization.getElapsedTime());
}