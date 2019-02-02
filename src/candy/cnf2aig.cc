/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <cassert>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <algorithm>

#include "candy/core/CNFProblem.h"
#include "candy/core/SolverTypes.h"
#include "candy/gates/GateAnalyzer.h"

using namespace Candy;

const Lit lit_True = { -3 };

class BinaryAndGate {
public:
    BinaryAndGate(Lit output_, Lit left_, Lit right_) :
        output(output_), left(left_), right(right_) {}

	Lit output;
	Lit left;
	Lit right;
};

class AndGate {
public:
    AndGate(Lit output_, Cl conj_) :
        output(output_), conj(conj_) {}

	Lit output;
    Cl conj;
};



CNFProblem problem;
GateAnalyzer* gates;
Lit root;
std::set<Var> inputs;

std::vector<BinaryAndGate> ands;
std::vector<AndGate> pands;

std::vector<Lit>* literals;
std::vector<bool>* visitedNodes;

int maxVariable = 0;
std::vector<bool>* negativeOutput;

int gaplessMax = 0;
std::map<Var, Var>* gapClosing = new std::map<Var, Var>();

int newVar() {
	return ++maxVariable;
}

bool isVisited(Lit node) {
	return (*visitedNodes)[toInt(node)];
}

void setVisited(Lit node) {
	(*visitedNodes)[toInt(node)] = true;
}

void createAnd(Lit output, Cl cube) {
    pands.emplace_back(output, cube);
}

void createBinaryAnd(Lit output, Lit left, Lit right) {
    ands.emplace_back(output, left, right);
}

void registerLiteral(Lit lit);
void createAndFromClause(Cl* disj, Lit output);
void createAndFromClauses(For& disjunctions, Lit output);

/**
 * Maintain Invariant: Mapping exists only when gate is defined
 */
void traverseDAG() {
	while (literals->size() > 0) {
		Lit lit = literals->back();
		literals->pop_back();

		if (isVisited(lit)) continue;
		setVisited(lit);

		if (gates->getGate(lit).isDefined()) {
			For& clauses = gates->getGate(lit).getForwardClauses();

			// strip output-literal from clauses
            For list;
			for (Cl* clause : clauses) {
				Cl* cl = new Cl(clause->size()-1);
				remove_copy(clause->begin(), clause->end(), cl->begin(), ~lit);
				list.push_back(cl);
			}

			createAndFromClauses(list, lit);

            for (Cl* c : list) delete c;
		}
	}
}

void createAndFromClauses(For& disjunctions, Lit output) {
    Cl conj;
	for (Cl* clause : disjunctions) {
		if (clause->size() == 1) {
			registerLiteral(*(clause->begin()));
			conj.push_back(*(clause->begin()));
		} else {
			Lit out = mkLit(newVar());
			createAndFromClause(clause, out);
			conj.push_back(out);
		}
	}

	createAnd(output, conj);
}

void createAndFromClause(Cl* disj, Lit output) {
    Cl conj;
    for (Lit lit : *disj) {
		registerLiteral(lit);
		conj.push_back(~lit);
	}

	createAnd(~output, conj);
}

void registerLiteral(Lit lit) {
	if (gates->getGate(lit).isDefined()) {
		literals->push_back(lit);
	} else if (gates->getGate(~lit).isDefined()) {
		literals->push_back(~lit);
	} else {
        inputs.insert(var(lit));
	}
}

Lit convertNaryRecursive(Cl* conj) {
	if (conj->size() == 1) {
		return *conj->begin();
	}
	if (conj->size() == 2) {
		Lit output = mkLit(newVar());
		createBinaryAnd(output, *conj->begin(), *conj->rbegin());
		return output;
	}
	else {
		int pivot = conj->size() / 2;
		Cl* lefts = new Cl(conj->begin(), conj->begin() + pivot);
		Lit left = convertNaryRecursive(lefts);
		delete lefts;
		Cl* rights = new Cl(conj->begin() + pivot, conj->end());
		Lit right = convertNaryRecursive(rights);
		delete rights;
		Lit output = mkLit(newVar());
		createBinaryAnd(output, left, right);
		return output;
	}
}

void convertNaryAndsToBinaryAnds() {
    for (AndGate a : pands) {
        if (a.conj.size() == 0) {
            fprintf(stderr, "Warning: Conjunction Size is Zero at %i\n ", toInt(a.output));
		}
        else if (a.conj.size() == 1) {
            createBinaryAnd(a.output, *a.conj.begin(), lit_True);
		}
        else if (a.conj.size() == 2) {
            createBinaryAnd(a.output, *a.conj.begin(), *a.conj.rbegin());
		}
		else {
            int pivot = a.conj.size() / 2;
            Cl* lefts = new Cl(a.conj.begin(), a.conj.begin() + pivot);
			Lit left = convertNaryRecursive(lefts);
			delete lefts;
            Cl* rights = new Cl(a.conj.begin() + pivot, a.conj.end());
			Lit right = convertNaryRecursive(rights);
			delete rights;
            createBinaryAnd(a.output, left, right);
		}
	}
}

/**
 * remember output negation
 */
Lit negOutAdaption(Lit lit) {
	return (*negativeOutput)[var(lit)] ? ~lit : lit;
}

/**
 * close gaps in variable numbers
 */
Lit closeGaps(Lit lit) {
	if (gapClosing->count(var(lit)) > 0) {
		return mkLit((*gapClosing)[var(lit)], sign(lit));
	}
	else {
		(*gapClosing)[var(lit)] = gaplessMax++;
		return mkLit((*gapClosing)[var(lit)], sign(lit));
	}
}

/**
 * print literal while selecting the proper sign and variable number
 */
void printLit(FILE* out, Lit lit) {
	if (lit == lit_True) {
		fprintf(out, "1");
	} else {
		int num = toInt(closeGaps(negOutAdaption(lit)));
		fprintf(out, "%i", num);
	}
}

int main(int argc, char** argv) {
	if (argc == 1) {
		fprintf(stderr, "Usage: cnf2aig [parameters] file\n");
		exit(1);
	}

	FILE* out = stdout;
    char* filename = argv[1];

    problem.readDimacsFromFile(filename);

    if (problem.nClauses() == 0) {
		fprintf(out, "aag 0 0 0 1 0\n1\n"); // trivial SAT
		exit(0);
	}
    else if (problem.nVars() == 0 || problem.hasEmptyClause()) {
		fprintf(out, "aag 0 0 0 1 0\n0\n"); // trivial UNSAT
		exit(0);
	}

    // todo: check for off-by-one (seems like overkill to add 2)
    visitedNodes = new std::vector<bool>((problem.nVars()+2)*2, false);

    gates = new GateAnalyzer(problem);
	gates->analyze();

    // todo: check for off-by-one (seems like overkill to add 1)
    maxVariable = problem.nVars()+1;

    literals = new std::vector<Lit>();
    Lit root = gates->normalizeRoots();
    literals->push_back(root);

    assert(var(root) < maxVariable);

	traverseDAG();

	convertNaryAndsToBinaryAnds();

	/*******
	 * As output may not be negative in AIG format we need to flip these globally:
	 ***/
	negativeOutput = new std::vector<bool>(maxVariable+2, false);
    for (BinaryAndGate a : ands) {
        if (sign(a.output)) (*negativeOutput)[var(a.output)] = true;
	}

	// **************
	// ** Print AIG ****
	// ***************

    fprintf(out, "aag %i %i %i %i %i\n", maxVariable+2, (int)inputs.size(), 0, 1, (int)ands.size());
    for (Var var : inputs) {
		printLit(out, mkLit(var));
		fprintf(out, "\n");
	}
	printLit(out, root);
	fprintf(out, "\n");

    for (BinaryAndGate a : ands) {
        printLit(out, a.output);
		fprintf(out, " ");
        printLit(out, a.left);
		fprintf(out, " ");
        printLit(out, a.right);
		fprintf(out, "\n");
	}
}


