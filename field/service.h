﻿

#ifndef SERVICE_H
#define SERVICE_H

#include "field_data.h"

class Service: public FieldData
{
public:
	
	Service(Field* field, const int row, const int column, const int type);
	
	
	/** All service fields types.*/
	enum
	{
		UNDEFINED,
		ID,	/**< Needed to link against this field from other (or current) tables.*/
	};
	
	/** Type of this field.*/
	int type;
	
	/** Depends on service field type. Surely not of "service" type.*/
	FieldData* fieldData;
};

#endif//#ifndef SERVICE_H

