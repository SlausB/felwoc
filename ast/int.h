

#ifndef INT_H
#define INT_H

#include "field_data.h"

class Int: public FieldData
{
public:
	
	Int(Field* field, const int value);
	
	int value;
};

#endif//#ifndef INT_H

