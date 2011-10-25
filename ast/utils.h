

#ifndef AST_UTILS_H
#define AST_UTILS_H

#include "table.h"

//std::function:
#include <functional>


/** Calls function for each table's parents including table itself.
\param current Will be passed to acceptor too.
\param acceptor Has to return true if it's need to keep iteration.*/
void ForEachTable(Table* current, std::function<bool(Table*)> acceptor);

#endif//#ifndef AST_UTILS_H

