#pragma once

#include <cstdio>

#include "./output.h"

#include <string>

/** Вывод в std::string.*/
class StringOutput: public Output
{
public:
	/** Изначальное состояние строки, в которую производится вывод.*/
	StringOutput(std::string start = "");
	
	void out(const char* msg, const int type);
	
	/** Сюда всё и выводится.*/
	std::string result;
};

