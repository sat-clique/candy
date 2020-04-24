#include <candy/core/SolverTypes.h>
#include <candy/core/CNFProblem.h>
#include <candy/core/CandySolverResult.h>
#include <candy/utils/Exceptions.h>
#include <candy/core/clauses/Certificate.h>
#include <candy/core/DRATChecker.h>

extern "C" {
#include <candy/ipasir/ipasir.h>
}


static int c2i(Candy::Lit lit) {
    return (lit.var() + 1) * (lit.sign() ? -1 : 1);
}

const char* CERT = "cert.drat";
Candy::Certificate certificate(CERT);

int main(int argc, char** argv) {
    std::cout << "c Candy is made from Glucose." << std::endl;
    std::cout << "c Ipasir Signature: " << ipasir_signature() << std::endl;

    if (argc == 1) return 0;
    const char* inputFilename = argv[1];
    std::cout << "c Reading file: " << inputFilename << std::endl; 

    Candy::CNFProblem problem {};
    try {
        problem.readDimacsFromFile(inputFilename);
    } 
    catch (Candy::ParserException& e) {
        std::cout << "c Caught Parser Exception: " << std::endl << e.what() << std::endl;
        return 1;
    }

    if (problem.nVars() > 100 || problem.nClauses() == 0) return 0;

    void* solver = ipasir_init();
    
    for (Candy::Cl* clause : problem) {
        for (Candy::Lit lit : *clause) {
            ipasir_add(solver, c2i(lit));
        }
        ipasir_add(solver, 0);
    }

    /***
     * Use literals of first clause as assumptions
     **/
    for (Candy::Lit lit : *problem[0]) {
        ipasir_assume(solver, c2i(lit));
    }    
    int result = ipasir_solve(solver);
    
    if (result == 10) {
        Candy::CandySolverResult model;
        for (Candy::Var var = 0; var < (Candy::Var)problem.nVars(); var++) {
            if (ipasir_val(solver, var+1) > 0) {
                model.setModelValue(Candy::Lit(var, false));
            } else {
                model.setModelValue(Candy::Lit(var, true));
            }
        }
        bool satisfied = problem.checkResult(model);
        if (satisfied) {
            std::cout << "c Result verified by model checker" << std::endl;
            std::cout << "c ********************************" << std::endl;
            return 0;
        }
        else {
            std::cout << "c Result could not be verified by model checker" << std::endl;
            assert(satisfied);
            return 1;
        }
    }
    else if (result == 20) {        
        for (Candy::Lit lit : *problem[0]) {
            if (ipasir_failed(solver, c2i(lit))) {
                problem.readClause({ lit });
            }
        }

        Candy::DRATChecker checker(problem);
        bool proved = checker.check_proof(CERT);
        if (proved) {
            std::cout << "c Result verified by proof checker" << std::endl;
            std::cout << "c ********************************" << std::endl;
            return 0;
        }
        else {
            std::cout << "c Result could not be verified by proof checker" << std::endl;
            assert(proved);
            return 1;
        }
    }

    ipasir_release(solver);
}