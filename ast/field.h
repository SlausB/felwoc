

#ifndef FIELD_H
#define FIELD_H

#include <string>

/** Data field type within table.*/
class Field
{
public:
	
	Field(const int type);
	
	
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
		BOOL,	/**< Boolean.*/
	};
	
	/** This fields type.*/
	int type;
	
	std::string name;
	
	/** Empty if it was NOT specified.*/
	std::string commentary;
};

#endif//#ifndef FIELD_H

