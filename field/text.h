

#ifndef TEXT_H
#define TEXT_H

#include "field_data.h"

class Text: public FieldData
{
public:
	
	Text(Field* field, const std::string& text);
	
	std::string text;
};

#endif//#ifndef TEXT_H

