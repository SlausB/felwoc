

#ifndef TABLE_H
#define TABLE_H

#include <vector>

#include "field.h"


/** Mathematical table with rows and columns.*/
class Table
{
public:
	
	Table();
	
	/** All possible table types.*/
	enum
	{
		UNTYPED,	/**< Error state or table was not defined yet.*/
		MANY,	/**< Many objects of different types.*/
		SINGLE,	/**< So-called "singleton" - single object which fields are table's rows values which can be changed and object can be serialized.*/
		PRECISE,	/**< The same as "SINGLE" but fields are fixed (cannot be changed), known right after loading (they are constant) and serialization cannot be applied.*/
		MORPH,	/**< Single object which can have one of states which are defined in rows.*/
		VIRTUAL,	/**< "Virtual table" from which someone can be inherited.*/
	};

	/** This table type.*/
	int type;
	
	/** Parent folder or NULL if this folder does not have any parent. Only single inheritance implemented yet.*/
	Table* parent;
	
	/** All tables fields. Inherited too.*/
	std::vector<Field*> fields;
};

#endif//#ifndef TABLE_H

