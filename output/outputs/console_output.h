
#ifndef CONSOLE_OUTPUT_H
#define CONSOLE_OUTPUT_H

#include <cstdio>

#include "./output.h"

/** Простой вывод в консоль.*/
class ConsoleOutput: public Output
{
public:
	void out(const char* msg, const int type);
};

#endif//#ifndef CONSOLE_OUTPUT_H

