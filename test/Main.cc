#include <zlib.h>
#include <sys/resource.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "../core/CNFProblem.h"
#include "simp/SimpSolver.h"
#include "gates/GateAnalyzer.h"

using namespace Glucose;

TEST (SolverTest, outputwithbudget) {
  SimpSolver S;

  CNFProblem dimacs;
  dimacs.readDimacsFromFile("vmpc_32.renamed-as.sat05-1919.cnf");

  S.insertClauses(dimacs);

  // create test output:
  S.setConfBudget(150000);
  S.verbosity++;
  S.verbEveryConflicts = 10000;
  vector<Lit> dummy;
  testing::internal::CaptureStdout();
  S.solveLimited(dummy);
  std::string output = testing::internal::GetCapturedStdout();

  // create reference output (comment out once generated):
//  FILE* f = fopen("/home/markus/git/candy-kingdom/test/reference.txt", "wb");
//  fprintf(f, "%s", output.c_str());
//  fflush(f);
//  fclose(f);

  // compare reference and test output:
  std::ifstream t("reference.txt");
  std::string reference((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  ASSERT_EQ (output, reference);
}

TEST (GateAnalyzerTest, countgates) {
  CNFProblem dimacs;
  dimacs.readDimacsFromFile("velev-vliw-uns-2.0-uq5.cnf");

  // create test out
  GateAnalyzer gates(dimacs);
  gates.analyze();
  testing::internal::CaptureStdout();
  printf("Number of gates: %7d", gates.getNGates());
  std::string output = testing::internal::GetCapturedStdout();

  // create reference output (comment out once generated):
//  FILE* f = fopen("/home/markus/git/candy-kingdom/test/countgates.txt", "wb");
//  fprintf(f, "%s", output.c_str());
//  fflush(f);
//  fclose(f);

  // compare reference and test output:
  std::ifstream t("countgates.txt");
  std::string reference((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  ASSERT_EQ (output, reference);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
