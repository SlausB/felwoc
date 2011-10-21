

#ifndef SERVICE_FIELD_H
#define SERVICE_FIELD_H

#include "field.h"

class ServiceField: public Field
{
public:
	ServiceField();
	
	/** All service fields types.*/
	enum
	{
		UNDEFINED,
		ID,	/**< Needed to link against this field from other (or current) tables.*/
	};
	
	/** Type of this service field.*/
	int serviceType;
};

#endif//#ifndef SERVICE_FIELD_H

