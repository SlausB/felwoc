

#ifndef ARRAY_H
#define ARRAY_H

#include "field_data.h"

#include <vector>


class Array: public FieldData
{
public:
	
	Array(Field* field, const int row, const int column);
	
	std::vector<double> values;
};

#endif//#ifndef ARRAY_H

