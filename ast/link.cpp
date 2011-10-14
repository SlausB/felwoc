

#include "link.h"


Count::Count(Table* table, const int id, const int count): table(table), id(id), count(count)
{
}

Link(Field* field, const int row, const int column, const std::string& text): FieldData(field, row, column), text(text)
{
}

