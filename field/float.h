

#ifndef FLOAT_H
#define FLOAT_H

#include "field_data.h"

class Float: public FieldData
{
public:
	
	Float(Field* field, const double value);
	
	double value;
};

#endif//#ifndef FLOAT_H

