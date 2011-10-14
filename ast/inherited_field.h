

#ifndef INHERITED_FIELD_H
#define INHERITED_FIELD_H

#include "table.h"
#include "field.h"

class InheritedField: public Field
{
public:
	
	InheritedField();
	
	
	/** Type of field within inherited parent - no recursion, so if this field inherited from grandparent - this is his field.*/
	Field* parentField;
	
	/** Table where this field was firstly introduced.*/
	Table* parent;
};

#endif//#ifndef INHERITED_FIELD_H

