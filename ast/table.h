

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
		UNTYPED,    /**< Error state or table was not defined yet.*/
		MANY,       /**< Many objects of different types.*/
		SINGLE,     /**< So-called "singleton" - single object which fields are table's rows values which can be changed and object can be serialized.*/
		PRECISE,    /**< The same as "SINGLE" but fields are fixed (cannot be changed), known right after loading (they are constant) and serialization cannot be applied.*/
		MORPH,      /**< Single object which can have one of states which are defined in rows.*/
		VIRTUAL,    /**< "Virtual table" from which something can be inherited.*/
	};

	/** This table type.*/
	int type;
	
	/** Parent folder or NULL if this folder does not have any parent. Only single inheritance implemented yet.*/
	Table* parent;
	
	/** All tables fields types. Inherited too. In the same order as within matrix. */
	std::vector<Field*> fields;
	
	/** All data fields in order [row][column] where columns in the same order as within fields. Can be empty.
	If table is of type PRECISE or SINGLE, then first dimension always has length of 1.*/
	std::vector<std::vector<FieldData*> > matrix;
	
	/** It's name within XLS. Used just for errors reporting.
	\sa lowercaseName */
	std::string realName;
	
	/** Used for linkage, inheritance and so on - "fxls" is case-insensitive.*/
	std::string lowercaseName;

	/** Commentary for whole table. Evaluate it as commentary for class.*/
	std::string commentary;


	/** Fields of table of type "precise".*/
	class Precise
	{
	public:
		enum
		{
			TYPE,	/**< Such as text, int, float, link...*/
			NAME,	/**< Field's name.*/
			VALUE,	/**< Field's value depending on it's type.*/
			COMMENTARY,	/**< Field's commentary.*/
		};
	};
};

#endif//#ifndef TABLE_H

