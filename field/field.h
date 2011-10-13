

#ifndef FIELD_H
#define FIELD_H

#include <string>

/** Data field type within table.*/
class Field
{
public:
	
	/** All possible field types.*/
	enum
	{
		UNDEFINED,	/**< Error.*/
		INHERITED,	/**< Inherited from parent table.*/
		SERVICE,	/**< Needed for implementation.*/
		TEXT,	/**< Literal.*/
		FLOAT,	/**< Real-valued.*/
		INT,	/**< Integer.*/
		LINK,	/**< Link to some other row.*/
		FIELD_NULL,	/**< Must NOT be compiled.*/
	};
	
	/** This fields type.*/
	int type;
	
	std::string name;
	
	/** Empty if it was NOT specified.*/
	std::string commentary;
};

#endif//#ifndef FIELD_H

