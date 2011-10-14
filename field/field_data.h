

#ifndef FIELD_DATA_H
#define FIELD_DATA_H

#include "field.h"


/** Must be casted to one of inheritants to use specific data dependently on field type.*/
class FieldData
{
public:
	
	FieldData(const int row, const int column, Field* field);
	
	/** Type of current data.*/
	Field* field;
	
	/** Row number (starting from 1) of field within XLS.*/
	int row;
	/** Column number (starting from 1) of field within XLS.*/
	int column;
};

#endif//#ifndef FIELD_DATA_H

