

#ifndef INHERITED_H
#define INHERITED_H

#include "field.h"

class Inherited: public Field
{
public:
	/** Defined within parent from which this field is inherited. No recursion, so if field inherited from great parent then this is his field.*/
	Field* field;
};

#endif//#ifndef INHERITED_H

