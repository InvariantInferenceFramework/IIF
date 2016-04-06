#include "config.h"
#include "color.h"
#include "polynomial.h"
#include "classifier.h"
#include "linear_learner.h"

#include <iostream>
//#include <float.h>
//#include <string.h>
#include <assert.h>

static void print_null(const char *s) {}

LinearLearner::LinearLearner(States* gsets, const char* solution_filename, 
		int (*func)(int*), int max_iteration) : BaseLearner(gsets, solution_filename, func) {
	svm = new SVM(0, print_null);
	this->max_iteration = max_iteration;
}

LinearLearner::~LinearLearner() {
	delete svm;
}


/// type == 0, solve polyations defined by paras....
//			   if params == NULL, all are random points
//			   if params != NULL, selective sampling
int LinearLearner::selectiveSampling(int randn, int exen, int type, void* params)
{
#ifdef __PRT
	std::cout << "{" << GREEN;
#endif

	if ((type != 0) && (exen > type))
		randn += exen - type;
	Solution input;
	int ret = 0;
	for (int i = 0; i < randn; i++) {
		Polynomial::solver(NULL, input);
		//model_solver(NULL, input);
		//Classifier::solver(NULL, input);
		ret = runTarget(input);

#ifdef __PRT
		std::cout << input;
		printRunResult(ret);
		std::cout << "|";
#endif

	}

#ifdef __PRT
	std::cout << BLUE;
#endif

	if (type == 0) {
		for (int i = 0; i < exen; i++) {
			//model_solver((svm_model*)params, input);
			Polynomial::solver((Polynomial*)params, input);
			//Classifier::solver((Classifier*)params, input);
			ret = runTarget(input);

#ifdef __PRT
			std::cout << "|" << input;
			printRunResult(ret);
#endif

		}
	}

#ifdef __PRT
	std::cout << WHITE << "}" << std::endl;
#endif

	return randn + exen;
}


int LinearLearner::learn()
{
	int rnd;
	bool similarLast = false;
	bool converged = false;
	svm_model* lastModel = NULL;
	int pre_psize = 0, pre_nsize = 0;

	double pass_rate = 1;
	int etimes = 1;

	int zero_times = 0;

	for (rnd = 1; ((rnd <= max_iteration) /*&& (pass_rate >= 1)*/); rnd++) {
		int nexe = (rnd == 1) ? Nexe_init : Nexe_after;
#ifdef __PRT
		int step = 1;
		std::cout << RED << "[" << rnd << "]" << WHITE;
		std::cout << RED << "Linear SVM------------------------{" << etimes 
			<< "}------------------------------------------------------------------------------------\n\t(" 
			<< YELLOW << step++ << WHITE << ") execute programs... [" << nexe + Nexe_rand << "] ";
#else
		std::cout << RED << "[" << rnd;
#endif

init_svm:
		//std::cout << std::endl << "\t-->selective sampling:\n\t";
		//selectiveSampling(random_exes, nexe, 0, lastModel);
		selectiveSampling(Nexe_rand, nexe, 0, svm->poly);
		//std::cout << "\t<--selective sampling:\n";

		if ((rnd == 1) && (gsets[POSITIVE].traces_num() == 0 || gsets[NEGATIVE].traces_num() == 0)) {

#ifdef __PRT
			if (gsets[POSITIVE].traces_num() == 0) 
				std::cout << RED << "\tZero Positive trace, execute program again." << WHITE << std::endl;
			if (gsets[NEGATIVE].traces_num() == 0) 
				std::cout << RED << "\tZero Negative trace, execute program again." << WHITE << std::endl;
#else
		std::cout << "+";
#endif

			zero_times++;
			if (zero_times >= Nretry_init)
				exit(-1);
			goto init_svm;
		}

#ifdef __PRT
		std::cout << "\t(" << YELLOW << step++ << WHITE << ") prepare training data... ";
#else
		std::cout << "]" << WHITE;
#endif

		svm->makeTrainingSet(gsets, pre_psize, pre_nsize);

		while (etimes <= 3) {
			cl->clear();

#ifdef __PRT
			std::cout << "\n\t(" << YELLOW << step++ << WHITE << ") start training "
				"with mapping dimension {" << etimes << "}...";
#endif

			svm->setEtimes(etimes);
			svm->train();

#ifdef __DS_ENABLED
			std::cout << "[" << svm->problem.np << ":" << svm->problem.nn << "]";
#endif

			std::cout << "|-->> " << YELLOW << *svm << WHITE << std::endl;
			//Polynomial poly;
			//svm->poly->roundoff(poly);
			//svm->poly->roundoff();
			//cl->factor(*(svm->poly));

#ifdef __PRT
			std::cout << "\t(" << YELLOW << step++ << WHITE << ") checking training traces.";
#endif

			pass_rate = svm->checkTrainingSet();

#ifdef __PRT
			if (pass_rate == 1)
				std::cout << GREEN << " [100%] [PASS]" << WHITE;
			else
				std::cout << RED << " [" << pass_rate * 100 << "%]" 
					<< " [FAIL] \n The problem is not linear separable by mapping "
					<< etimes << ".. Trying to project to a higher space " << WHITE << std::endl;
#endif

			if (pass_rate == 1)
				break;
			etimes++;
		}
		if (etimes > 3) break;

		/*
		 *	similarLast is used to store the convergence check return value for the last time.
		 *	We only admit convergence if the three consecutive round are converged.
		 *	This is to prevent in some round the points are too right to adjust the classifier.
		 */
#ifdef __PRT
		std::cout << "\n\t(" << YELLOW << step++ << WHITE << ") check convergence:        ";
#endif

		if (svm->converged_model() == true) {
			if (similarLast == true) {
#ifdef __PRT
				std::cout << "[TT]  [SUCCESS] rounding off" << std::endl;
#endif
				converged = true;
				rnd++;
				break;
			}

#ifdef __PRT
			std::cout << "[FT]";
			Nexe_after *= 2;
#endif

			similarLast = true;
		} else {

#ifdef __PRT
			std::cout << ((similarLast == true) ? "[T" : "[F") << "F] ";
			if (similarLast == true)
				Nexe_after /= 2;
#endif

			similarLast = false;
		}
#ifdef __PRT
		std::cout << "  [FAIL] neXt round " << std::endl;
#endif

		//lastModel = svm->getModel();
		lastModel = svm->model;
	} // end of SVM training procedure

	std::cout << "----------------------------------------------------------------------------" 
		<< "----------------------------------------\n";

	int ret = 1;
	if ((converged) && (rnd <= max_iteration)) {
		svm_model_visualization(lastModel, svm->poly);
		//ret = poly->toCandidates(cs);
		//poly->roundoff();
		//svm_model_approximate(lastModel, poly->getEtimes());
		//svm_problem_approximate(&svm->problem, poly->getEtimes());
		//std::cout << GREEN << "generated model" << *lastModel << std::endl << WHITE;
		//poly->roundoff();
		std::cout << YELLOW << "  Invariant Candidate: {  ";
		std::cout << GREEN << svm->poly->toString() << YELLOW;
		std::cout << "  }" << WHITE << std::endl;
	}

	if ((pass_rate < 1) || (rnd >= max_iteration)) {
		std::cout << RED << "  Cannot divide by SVM perfectly.\n" << WHITE;
		ret = -1;
	}

#ifndef _multi_candidates_
	if (ret > 0)
		return 1;
#endif

	return ret;
}

std::string LinearLearner::invariant(int n) {
	svm->poly->roundoff();
	cl->factor(*svm->poly);
	return cl->toString();
	return svm->poly->toString();
	//return cs->toString(n);
	//return cl->toString();
}
