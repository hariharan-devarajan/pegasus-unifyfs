//
// Created by haridev on 4/24/23.
//

#ifndef UNIFYFS_PEGASUS_TEST_CACHE_CONFIG_H
#define UNIFYFS_PEGASUS_TEST_CACHE_CONFIG_H

#define CATCH_CONFIG_RUNNER

#include <catch2/catch_all.hpp>
// #include <mpi.h>

namespace cl = Catch::Clara;

cl::Parser define_options();

int init(int *argc, char ***argv);
int finalize();

#define CONVERT_ENUM(name, value) \
  "[" + std::string(#name) + "=" + std::to_string(static_cast<int>(value)) + "]"

#define CONVERT_VAL(name, value) \
  "[" + std::string(#name) + "=" + std::to_string(value) + "]"
#define CONVERT_STR(name, value) \
  "[" + std::string(#name) + "=" + std::string(value) + "]"

#define DEFINE_CLARA_OPS(TYPE)                                 \
  std::ostream &operator<<(std::ostream &out, const TYPE &c) { \
    out << static_cast<int>(c) << std::endl;                   \
    return out;                                                \
  }                                                            \
  TYPE &operator>>(const std::stringstream &out, TYPE &c) {    \
    c = static_cast<TYPE>(atoi(out.str().c_str()));            \
    return c;                                                  \
  }
#define AGGREGATE_TIME(name)                                      \
  double total_##name = 0.0;                                      \
  auto name##_a = name##_time.getElapsedTime();                   \
  MPI_Reduce(&name##_a, &total_##name, 1, MPI_DOUBLE, MPI_SUM, 0, \
             MPI_COMM_WORLD);


int main(int argc, char *argv[]) {
  // tfs_init();
  Catch::Session session;
  auto cli = session.cli() | define_options();
  session.cli(cli);
  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0) return returnCode;
  returnCode = init(&argc, &argv);
  if (returnCode != 0) return returnCode;
  int test_return_code = session.run();
  returnCode = finalize();
  if (returnCode != 0) return returnCode;
  // tfs_finalize();
  exit(test_return_code);
}
#endif //UNIFYFS_PEGASUS_TEST_CACHE_CONFIG_H
