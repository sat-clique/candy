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

TEST (TestTest, zeqz) {
  ASSERT_EQ (0.0, 0.0);
}

TEST (SolverTest, outputwithbudget) {
  SimpSolver S;
  gzFile in = gzopen("vmpc_32.renamed-as.sat05-1919.cnf", "rb");
  CNFProblem dimacs(in);
  gzclose(in);

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
  gzFile in = gzopen("velev-vliw-uns-2.0-uq5.cnf", "rb");
  CNFProblem dimacs(in);
  gzclose(in);

  // create test out
  GateAnalyzer gates(dimacs);
  gates.analyze();
  int g = gates.getNGates();
  testing::internal::CaptureStdout();
  printf("c |  Number of gates:      %12d                                                                   |\n", g);
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

TEST (GateAnalyzerTest2, printgates) {
  gzFile in = gzopen("velev-vliw-uns-2.0-uq5.cnf", "rb");
  CNFProblem dimacs(in);
  gzclose(in);

  // create test out
  GateAnalyzer gates(dimacs);
  gates.analyze();
  For* g = gates.getGates();
  testing::internal::CaptureStdout();
  int i = 0;
  for (Cl* c : *g) {
	  i++;
//	  if (i > 86810) break;
	  if (c == nullptr) continue;
	  if (c->size() > 0) printf("%i: ", i);
	  for (Lit l : *c) printf("%s%i ", sign(l)?"-":"", var(l)+1);
	  if (c->size() > 0) printf("\n");
  }
  std::string output = testing::internal::GetCapturedStdout();

//   create reference output (comment out once generated):
//  FILE* f = fopen("/home/markus/git/candy-kingdom/test/printgates.txt", "wb");
//  fprintf(f, "%s", output.c_str());
//  fflush(f);
//  fclose(f);

  // compare reference and test output:
  std::ifstream t("printgates.txt");
  std::string reference((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  ASSERT_EQ (output, reference);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
