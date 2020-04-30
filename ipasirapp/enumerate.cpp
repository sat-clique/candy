/*
 * ipasir-example.cpp
 *
 *  Created on: April 30, 2020
 *      Author: Markus Iser, KIT
 */

extern "C" {
    #include "ipasir.h"
}

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <assert.h>

/**
 * Reads a formula from a given file 
 */
bool loadFormula(void* solver, const char* filename, int* outVariables) {
	FILE* f = fopen(filename, "r");

	if (f == NULL) {
		return false;
	}

	int maxVar = 0;
	int c = 0;
	bool neg = false;	
	while (c != EOF) {
		c = fgetc(f);

		if (c == 'c' || c == 'p') { // skip comments and header
			while(c != '\n') c = fgetc(f);
			continue;
		}
		
		if (isspace(c)) continue; // skip whitespace
		
		if (c == '-') { // negative number coming
			neg = true;
			continue;
		}

		// number
		if (isdigit(c)) {
			int num = c - '0';
			c = fgetc(f);
			while (isdigit(c)) {
				num = num*10 + (c-'0');
				c = fgetc(f);
			}
			if (neg) {
				num *= -1;
			}
			neg = false;

			if (abs(num) > maxVar) {
				maxVar = abs(num);
			}
			// add to the solver
			ipasir_add(solver, num);
		}
	}

	fclose(f);
	*outVariables = maxVar;
	return true;
}


int main(int argc, char **argv) {
	std::cout << "cc Using the incremental SAT solver" << ipasir_signature() << std::endl;

	if (argc != 2) {
		puts("cc USAGE: ./enumerate <dimacs.cnf>");
		return 0;
	}

	void *solver = ipasir_init();
	int variables = 0;
	bool loaded = loadFormula(solver, argv[1], &variables);

	if (!loaded) {
		std::cout << "cc The input formula " << argv[1] << " could not be loaded." << std::endl;
		return 0;
	}
	else {
		std::cout << "cc Loaded, solving" << std::endl;
	}

	int satRes = ipasir_solve(solver);

	if (satRes == 20) {
		puts("The input formula is unsatisfiable.");
		return 0;
	}

	int count = 1;
	std::vector<int> model;
	while (satRes == 10) {
		// std::cout << "c Found " << count << " solutions" << std::endl;
		std::cout << "v ";
		for (int var = 1; var <= variables; var++) {
			int value = ipasir_val(solver, var);
			assert(abs(value) == var);
			std::cout << value << " ";
			model.push_back(value);
		}
		std::cout << std::endl;
		for (int lit : model) ipasir_add(solver, -lit);
		ipasir_add(solver, 0);
		satRes = ipasir_solve(solver);
		count++;
		model.clear();
	}
}
