

#ifndef AST_H
#define AST_H

#include <vector>

#include "table.h"
#include "field.h"
#include "inherited_field.h"
#include "link.h"
#include "service.h"
#include "inherited.h"
#include "int.h"
#include "float.h"
#include "text.h"
#include "bool.h"
#include "array.h"

/** "Abstract Syntax Tree" which is a result of XLS parsing.*/
class AST
{
public:

	/** Name of XLS file from which this "abstract syntax tree" was generated.*/
	std::string fileName;

	/** All parsed tables. Can be inherited from each other.*/
	std::vector<Table*> tables;
};

#endif//#ifndef AST_H

