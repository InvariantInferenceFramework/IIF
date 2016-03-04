#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include "instrumentation.h"
#include <assert.h>

bool _passP = false;
bool _passQ = false;
int assume_times = 0;
int assert_times = 0;
char lt[4][10] =  { "Negative", "Question", "Positive", "Bugtrace"};
char(*LabelTable)[10] = &lt[1];

double program_states[MstatesIn1trace][Nv];
int state_index;

#include "color.h"
int addStateInt(int first ...)
{
	va_list ap;
	va_start(ap, first);
	program_states[state_index][0] = first;
	for (int i = 1; i < Nv; i++) {
		program_states[state_index][i] = va_arg(ap, int);
	}
	va_end(ap);

#ifdef __PRT_TRACE
	std::cout << BLUE << "(" << program_states[state_index][0];
	for (int i = 1; i < VARS; i++) {
	    std::cout << "," << program_states[state_index][i];
	}
	std::cout << ")" << WHITE;
#endif
	state_index++;
	if (state_index >= MstatesIn1trace) {
		std::cout << RED << "\nToo many states (>" << MstatesIn1trace << ") in one execution. Stop here.\n" << WHITE;
		exit(-1);
	}
	return 0;
}

int addStateDouble(double first, ...)
{
	va_list ap;
	va_start(ap, first);
	program_states[state_index][0] = first;
	for (int i = 1; i < Nv; i++) {
		program_states[state_index][i] = va_arg(ap, double);
	}
	va_end(ap);
	state_index++;
	if (state_index >= MstatesIn1trace) {
		std::cout << RED << "\nToo many states (>" << MstatesIn1trace << ") in one execution. Stop here.\n" << WHITE;
		exit(-1);
	}
	return 0;
}


int beforeLoop()
{
	//std::cout << "---> before_loop";
	state_index = 0;
	_passP = false;
	_passQ = false;
	assume_times = 0;
	assert_times = 0;
	//std::cout << "[done]";
	return 0;
}


int afterLoop(States* gsets)
{
	int label = 0;
	assert(assume_times == 1);
	assert(assert_times == 1);
	if (_passP && _passQ) {
		label = 1;
	} else if (!_passP && !_passQ) {
		label = -1; 
	} else if (!_passP && _passQ) {
		label = 0; 
	} else if (_passP && !_passQ) {
		label = 2;
	}

#ifdef __PRT_TRACE
	std::cout << BLUE << "TRACE: ";
	for (int i = 0; i < state_index; i++) {
		std::cout << "(" << program_states[i][0];
		for (int j = 1; j < D; j++)
			std::cout << "," << program_states[i][j];
		std::cout << ")->";
	}
	std::cout << "END[" << label << "]" << WHITE << std::endl;
#endif
	
	gsets[label].addStates(program_states, state_index);
	return label;
}

void printRunResult(int rr) {
	switch (rr) {
		case -1:
			std::cout << "-";
			break;
		case 0:
			std::cout << "?";
			break;
		case 1:
			std::cout << "+";
			break;
		case 2:
			std::cout << "x";
			break;
	}
}

int mDouble(double* p)
{
	int a[Nv];
	for (int i = 0; i < Nv; i++)
		a[i] = static_cast<int>(p[i]);
	return mInt(a);
}

int mInt(int* p) { return target_program(p); }
