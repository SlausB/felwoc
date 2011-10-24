

#ifndef BOOL_H
#define BOOL_H

#include "field_data.h"

class Bool: public FieldData
{
public:
	
	Bool(Field* field, const int row, const int column, const bool value);
	
	bool value;
};

#endif//#ifndef BOOL_H

