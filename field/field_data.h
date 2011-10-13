

#ifndef FIELD_DATA_H
#define FIELD_DATA_H

#include "field.h"


class FieldData
{
public:
	
	FieldData(Field* field);
	
	/** Type of current data.*/
	Field* field;
};

#endif//#ifndef FIELD_DATA_H

