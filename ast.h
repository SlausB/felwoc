

#ifndef AST_H
#define AST_H

#include <vector>

#include "table.h"

/** "Abstract Syntax Tree" which is a result of XLS parsing.*/
class AST
{
public:
	/** All parsed tables. Can be inherited from each other.*/
	std::vector<Table*> tables;
};

#endif//#ifndef AST_H

