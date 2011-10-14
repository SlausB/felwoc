

#ifndef LINK_H
#define LINK_H

#include <vector>

#include "field_data.h"
#include "table.h"


/** Used within link when reference has "count" attribute.*/
class Count
{
public:
	
	Count(Table* table, const int id, const int count);
	
	
	/** To item (spell it row) of which table this link is.*/
	Table* table;
	
	/** Id of a linked row within it's table.*/
	int id;
	
	/** "count" attribute of a link. If link don't have count attribute then 1.*/
	int count;
};

class Link: public FieldData
{
public:
	
	Link(Field* field, const int row, const int column, const std::string& text);
	
	
	/** Link's text as it is within XLS.*/
	std::string text;
	
	/** Link can be to multiple objects separated with commas.*/
	std::vector<Count> links;
};

#endif//#ifndef LINK_H

