

#ifndef FIELD_H
#define FIELD_H

#include <string>

/** Data field within table.*/
class Field
{
public:
	
	/** All possible field types.*/
	enum
	{
		UNDEFINED,	/**< Error.*/
		INHERITED,	/**< Inherited from parent table.*/
		TEXT,	/**< Literal.*/
		FLOAT,	/**< Real-valued.*/
		INT,	/**< Integer.*/
		LINK,	/**< Link to some other row.*/
	};
	
	/** This fields type.*/
	int type;
	
	std::string name;
};

#endif//#ifndef FIELD_H

