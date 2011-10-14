

#include "inherited_field.h"

InheritedField::InheritedField(Field* parentField, Table* parent): Field(Field::INHERITED), parentField(parentField), parent(parent)
{
}

