

#include "table.h"

#include <algorithm>


Table::Table(const std::string& name): realName(name), lowercaseName(name), type(Table::UNTYPED), parent(NULL)
{
	std::transform(lowercaseName.begin(), lowercaseName.end(), lowercaseName.begin(), ::tolower);
}

