

#ifndef SERVICE_H
#define SERVICE_H

#include "field_data.h"

#include "service_field.h"


class Service: public FieldData
{
public:
	
	Service(ServiceField* serviceField, const int row, const int column);
	
	
	/** Depends on service field type. Surely not of "service" type.*/
	FieldData* fieldData;
};

#endif//#ifndef SERVICE_H

