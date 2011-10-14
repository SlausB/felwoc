

#ifndef TABLE_H
#define TABLE_H

#include <vector>

#include "field.h"

#include "field_data.h"


/** Mathematical table with rows and columns (spreadsheet or worksheet).*/
class Table
{
public:
	
	Table(const std::string& name);
	
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
	
	/** All tables fields types. Inherited too. In the same order as within matrix.*/
	std::vector<Field*> fields;
	
	/** All data fields in order [row][column] where columns in the same order as within fields. Can be empty.*/
	std::vector<std::vector<FieldData*> > matrix;
	
	/** It's name within XLS. Used just for errors reporting.
	\sa lowercaseName */
	std::string realName;
	
	/** Used for linkage, inheritance and so on - "fxls" is case-insensitive.*/
	std::string lowercaseName;
};

#endif//#ifndef TABLE_H

