

#ifndef FLOAT_H
#define FLOAT_H

#include "field_data.h"

class Float: public FieldData
{
public:
	
	Float(Field* field, const int row, const int column, const double value);
	
	double value;
};

#endif//#ifndef FLOAT_H

