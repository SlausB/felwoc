﻿

#ifndef INHERITED_H
#define INHERITED_H

#include "field.h"
#include "field_data.h"

class Inherited: public FieldData
{
public:
	
	Inherited(Field* field, const int row, const int column, FieldData* fieldData);
	
	
	/** No recursion, so if field inherited from great parent then this is his field.*/
	FieldData* fieldData;
};

#endif//#ifndef INHERITED_H

