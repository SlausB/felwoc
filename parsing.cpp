

#include "parsing.h"

#include <boost/algorithm/string.hpp>

#include <map>
#include <set>

#include <algorithm>

#include <stdexcept>

#include "ast/ast.h"

#include <boost/program_options/detail/convert.hpp>

#include <boost/lexical_cast.hpp>

#include <sstream>

#include <assert.h>



#define FOR_EACH_WORKSHEET(name) \
	for(size_t worksheetIndex = 0; worksheetIndex < worksheets.size(); worksheetIndex++) \
	{ \
		WorksheetTable* name = worksheets[worksheetIndex]; \
		if(name->table == NULL) \
		{ \
			continue; \
		}


int ToInt(const double value)
{
    if(value >= 0.0f) return (int)(value + 0.5f);
    else return (int)(value - 0.5f);
}

/** Forms Excel-like (such as "ABK") column name.*/
std::string PrintColumn(const int columnIndex)
{
	const int base = 'Z' - 'A' + 1;

	int currentValue = columnIndex + 1;
	std::string inverted;
	for(;;)
	{
		const int overflow = currentValue / base;
		const int remainder = currentValue % base;
		inverted.push_back('A' + remainder - 1);
		if(overflow > 0)
		{
			currentValue = overflow;
		}
		else
		{
			break;
		}
	}

	std::string normal;
	for(int i = (int)inverted.size() - 1; i >= 0 ; i--)
	{
		normal.push_back(inverted[i]);
	}

	return normal;
}

std::string ToChar(const wchar_t* source)
{
	/*//const int codePage = CP_ACP;
	const int codePage = CP_UTF8;
	
	const int result = WideCharToMultiByte(codePage, 0, source, -1, CHAR_BUFFER, sizeof(CHAR_BUFFER), NULL, NULL);
	if(result <= 0)
	{
		MSG(boost::format("E: WideCharToMultiByte() failed.\n"));
		return NULL;
	}
	return CHAR_BUFFER;*/
	
	std::wstring temp = source;
	return boost::to_utf8(temp);
}

int Keywords::Match(Messenger& messenger, const std::string& tableName, const int rowIndex, const int columnIndex, const std::string& keyword) const
{
	std::string lowercase = keyword;
	std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);

	std::map<std::string, int>::const_iterator it = keywords.find(lowercase);
	if(it == keywords.end())
	{
		bool notFirst = true;
		MSG(boost::format("E: keyword \"%s\" (originating from \"%s\") at row %d and column %d (%s) within table \"%s\" is undefined. Possible keywords: %s.\n") % lowercase % keyword % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % tableName % PrintPossible());
		return -1;
	}
	else
	{
		return it->second;
	}
}

void Keywords::Add(const std::string& keyword, const int match)
{
	keywords[keyword] = match;
}

std::string Keywords::Find(Messenger& messenger, const int value)
{
	for(std::map<std::string, int>::const_iterator it = keywords.begin(); it != keywords.end(); it++)
	{
		if(it->second == value)
		{
			return it->first;
		}
	}

	MSG(boost::format("E: PROGRAM ERROR: Keywords::Find(): value = %d is undefined.\n") % value);

	return std::string();
}

std::string Keywords::PrintPossible() const
{
	std::string result;
	bool notFirst = false;
	for(std::map<std::string, int>::const_iterator it = keywords.begin(); it != keywords.end(); it++)
	{
		if(notFirst)
		{
			result.append(", ");
		}
		else
		{
			notFirst = true;
		}
		
		result.append(str(boost::format("\"%s\"") % it->first));
	}
	return result;
}

class TableParamsKeywords
{
public:
	enum
	{
		TYPE,
		PARENT,
	};
};

class WorksheetTable
{
public:
	
	WorksheetTable(Messenger& messenger, Table* table, ExcelFormat::BasicExcelWorksheet* worksheet, const std::string& name): table(table), worksheet(worksheet), name(name), parent(NULL), processed(false)
	{
		//reduce rows and columns count:
		const int librariesRowsCount = worksheet->GetTotalRows();
		const int librariesColumnsCount = worksheet->GetTotalCols();

		//rows count:
		int rowIndex = librariesRowsCount - 1;
		for(; rowIndex >= 0; rowIndex--)
		{
			bool emptyRow = true;

			for(int columnIndex = 0; columnIndex < librariesColumnsCount; columnIndex++)
			{
				ExcelFormat::BasicExcelCell* cell = worksheet->Cell(rowIndex, columnIndex);
				if(cell->Type() != ExcelFormat::BasicExcelCell::UNDEFINED)
				{
					emptyRow = false;
					break;
				}
			}

			if(emptyRow == false)
			{
				break;
			}
		}
		rowsCount = rowIndex + 1;

		//columns count:
		int columnIndex = librariesColumnsCount - 1;
		for(; columnIndex >= 0; columnIndex--)
		{
			bool emptyColumn = true;

			for(int rowIndex = 0; rowIndex < librariesRowsCount; rowIndex++)
			{
				ExcelFormat::BasicExcelCell* cell = worksheet->Cell(rowIndex, columnIndex);
				if(cell->Type() != ExcelFormat::BasicExcelCell::UNDEFINED)
				{
					emptyColumn = false;
					break;
				}
			}

			if(emptyColumn == false)
			{
				break;
			}
		}
		columnsCount = columnIndex + 1;
	}
	
	/** Returns field accordingly toggled columns or NULL if column index is wrong or it was skipped.*/
	Field* FieldFromColumn(Messenger& messenger, const int columnIndex)
	{
		int skipped = 0;
		for(std::set<int>::iterator it = columnToggles.begin(); it != columnToggles.end(); it++)
		{
			const int skippedColumnIndex = (*it);
			if(skippedColumnIndex == columnIndex)
			{
				return NULL;
			}
			
			if(skippedColumnIndex < columnIndex)
			{
				skipped++;
			}
		}
		
		const int resultIndex = columnIndex - skipped - Parsing::COLUMN_MIN_COLUMN;
		if(resultIndex < 0)
		{
			MSG(boost::format("E: PROGRAM ERROR: resultIndex = %d is wrong. Refer to software supplier.\n") % resultIndex);
			return NULL;
		}
		if(resultIndex >= (int)table->fields.size())
		{
			MSG(boost::format("E: PROGRAM ERROR: resultIndex = %d is more that fields.size() = %u. Refer to software supplier.\n") % resultIndex % table->fields.size());
			return NULL;
		}
		return table->fields[resultIndex];
	}
	
	Table* table;
	
	std::string parentName;
	
	ExcelFormat::BasicExcelWorksheet* worksheet;
	
	WorksheetTable* parent;
	
	/** Turned off columns - they must NOT be compiled.*/
	std::set<int> columnToggles;
	
	/** All links collected during data parsing - have to be processed after it.*/
	std::vector<Link*> links;
	
	/** true if table's fields array was filled and it can be used within derived classes.*/
	bool processed;

	/** Worksheet's name. The same as table->realName if worksheet must be compiled, otherwise table is not even defined.*/
	std::string name;

	/** Due too weird behaviour of ExcelFormat library columns sometimes at one more then there really are, so this value truncated to max defined cells within constructor.*/
	int rowsCount;
	int columnsCount;
};

struct DerivedField
{
	Field* field;
	WorksheetTable* worksheetTable;
};



Parsing::Parsing()
{
	tableParamsKeywords.Add("type", TableParamsKeywords::TYPE);
	tableParamsKeywords.Add("parent", TableParamsKeywords::PARENT);
	
	tableTypesKeywords.Add("many", Table::MANY);
	tableTypesKeywords.Add("single", Table::SINGLE);
	tableTypesKeywords.Add("precise", Table::PRECISE);
	tableTypesKeywords.Add("morph", Table::MORPH);
	tableTypesKeywords.Add("virtual", Table::VIRTUAL);
	
	fieldKeywords.Add("service", Field::SERVICE);
	fieldKeywords.Add("text", Field::TEXT);
	fieldKeywords.Add("float", Field::FLOAT);
	fieldKeywords.Add("int", Field::INT);
	fieldKeywords.Add("link", Field::LINK);
	fieldKeywords.Add("bool", Field::BOOL);
	fieldKeywords.Add("array", Field::ARRAY);
	
	serviceFieldsKeywords.Add("id", ServiceField::ID);

	preciseTableKeywords.Add("type", Table::Precise::TYPE);
	preciseTableKeywords.Add("name", Table::Precise::NAME);
	preciseTableKeywords.Add("value", Table::Precise::VALUE);
	preciseTableKeywords.Add("commentary", Table::Precise::COMMENTARY);
}

std::vector<std::string> Parsing::Detach(const std::string& what, const char* delimiters)
{
	std::vector<std::string> strs;
	boost::split(strs, what, boost::is_any_of(delimiters));
	for(size_t i = 0; i < strs.size(); i++)
	{
		Cleanup(strs[i]);
		std::transform(strs[i].begin(), strs[i].end(), strs[i].begin(), ::tolower);
	}
	return strs;
}

void Parsing::Cleanup(std::string& what)
{
	for(size_t i = 0; i < what.size(); i++)
	{
		if(what[i] != ' ' && what[i] != '\n')
		{
			what.erase(0, i);
			break;
		}
	}
	
	for(int i = (int)what.size() - 1; i >= 0; i--)
	{
		if(what[i] != ' ' && what[i] != '\n')
		{
			const int startingSpace = i + 1;
			if(startingSpace < (int)what.size())
			{
				what.erase(startingSpace, std::string::npos);
			}
			break;
		}
	}
}

/** Returns string if cell is of type STRING of WSTRING, otherwise returns NULL.*/
std::string GetString(ExcelFormat::BasicExcelCell* cell)
{
	switch(cell->Type())
	{
	case ExcelFormat::BasicExcelCell::INT:
		return boost::lexical_cast<std::string>(cell->GetInteger());
	
	case ExcelFormat::BasicExcelCell::DOUBLE:
		return boost::lexical_cast<std::string>(cell->GetDouble());

	case ExcelFormat::BasicExcelCell::STRING:
		return cell->GetString();
	
	case ExcelFormat::BasicExcelCell::WSTRING:
		return ToChar(cell->GetWString());
	
	case ExcelFormat::BasicExcelCell::FORMULA:
		const char* formulaAsString = cell->GetString();
		//literal formulas contains resulting string:
		if(strlen(formulaAsString) > 0)
		{
			return formulaAsString;
		}
		//numerical formulas interpreted as empty string if to obtain them as string:
		return boost::lexical_cast<std::string>(cell->GetDouble());
	}
	
	return std::string();
}

/** Throws exception if cell cannot be casted to integer.*/
int GetInt(ExcelFormat::BasicExcelCell* cell)
{
	switch(cell->Type())
	{
	case ExcelFormat::BasicExcelCell::INT:
		return cell->GetInteger();
	
	case ExcelFormat::BasicExcelCell::DOUBLE:
		return ToInt(cell->GetDouble());
	
	case ExcelFormat::BasicExcelCell::FORMULA:
		{
			const char* formulaAsString = cell->GetString();
			//numerical formulas interpreted as empty string if to obtain them as string:
			if(strlen(formulaAsString) <= 0)
			{
				return ToInt(cell->GetDouble());
			}
			//if value is represented using literals concatination or something else:
			else
			{
				return ToInt(boost::lexical_cast<double, const char*>(formulaAsString));
			}
		}
		break;

	default:
		std::string asString = GetString(cell);
		if(asString.empty() == false)
		{
			return ToInt(boost::lexical_cast<double, std::string>(asString));
		}
		break;
	}
	
	throw std::runtime_error("cannot be casted to int");
}

/** Throws exception if cell cannot be casted to float.*/
double GetFloat(ExcelFormat::BasicExcelCell* cell)
{
	switch(cell->Type())
	{
	case ExcelFormat::BasicExcelCell::INT:
		return cell->GetInteger();
	
	case ExcelFormat::BasicExcelCell::DOUBLE:
		return cell->GetDouble();
	
	case ExcelFormat::BasicExcelCell::FORMULA:
		{
			const char* formulaAsString = cell->GetString();
			//literal formulas represented as empty string it to obtain them as string:
			if(strlen(formulaAsString) <= 0)
			{
				return cell->GetDouble();
			}
			else
			{
				return boost::lexical_cast<double, const char*>(formulaAsString);
			}
		}
		break;

	default:
		std::string asString = GetString(cell);
		if(asString.empty() == false)
		{
			return boost::lexical_cast<double, std::string>(asString);
		}
		break;
	}
	
	throw std::runtime_error("cannot be casted to float");
}

/** Recursively checks if inheritance has recursion. Returns true at some error.*/
bool WatchRecursiveInheritance(Messenger& messenger, std::vector<WorksheetTable*>& chain, WorksheetTable* wt)
{
	for(size_t i = 0; i < chain.size(); i++)
	{
		if(chain[i] == wt)
		{
			MSG(boost::format("E: found recursive inheritance with table \"%s\".\n") % wt->table->realName);
			return true;
		}
	}
	
	if(wt->parent == NULL)
	{
		return false;
	}
	chain.push_back(wt);
	return WatchRecursiveInheritance(messenger, chain, wt->parent);
}

/** Calls function for each table's parents including table itself.
\param current Will be passed to acceptor too.
\param acceptor Has to return true if it's need to keep iteration.*/
void ForEachTable(WorksheetTable* current, std::function<bool(WorksheetTable*)> acceptor)
{
	for(;;)
	{
		if(acceptor(current) == false)
		{
			return;
		}
		
		if(current->parent == NULL)
		{
			return;
		}
		else
		{
			current = current->parent;
		}
	}
}

/** Recursively parses columns types from parents tables to children.
\return false on some error.*/
bool ProcessColumnsTypes(Messenger& messenger, WorksheetTable* worksheet, Parsing* parsing)
{
	//because this function is called from parsing for each table AND it calls himself due to inheritance - it can be called several times for single table:
	if(worksheet->processed)
	{
		return true;
	}

	//relation for such tables types is inverted - columns specification is read for each row:
	if(worksheet->table->type == Table::PRECISE || worksheet->table->type == Table::SINGLE)
	{
		return true;
	}

	//if table has some parent then it can be processed only if parent already was, so process it first if it wasn't yet:
	if(worksheet->parent != NULL)
	{
		if(worksheet->parent->processed == false)
		{
			if(ProcessColumnsTypes(messenger, worksheet->parent, parsing) == false)
			{
				return false;
			}
		}
	}
	
	Table* table = worksheet->table;
	
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < worksheet->columnsCount; columnIndex++)
	{
		//if current column must not be compiled:
		if(worksheet->columnToggles.find(columnIndex) != worksheet->columnToggles.end())
		{
			continue;
		}

		const int rowIndex = Parsing::ROW_FIELDS_TYPES;

		ExcelFormat::BasicExcelCell* cell = worksheet->worksheet->Cell(rowIndex, columnIndex);
		if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
		{
			MSG(boost::format("E: column's type at row %d and column %d (%s) within table \"%s\" is undefined.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName);
			return false;
		}
		
		std::string typeName = GetString(cell);
		if(typeName.empty())
		{
			MSG(boost::format("E: cell at row %d and column %d (%s) within table \"%s\" is NOT of literal type. It has to be on of the: %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % parsing->fieldKeywords.PrintPossible());
			return false;
		}
		
		switch(parsing->fieldKeywords.Match(messenger, worksheet->table->realName, rowIndex, columnIndex, typeName))
		{
		case -1:
			return false;
			
		/*case Field::INHERITED:
			{
				if(worksheet->parent == NULL)
				{
					MSG(boost::format("E: table \"%s\" inherits field at column %d but it does NOT have any parent.\n") % worksheet->table->realName % (columnIndex + 1));
					return false;
				}

				InheritedField* inheritedField = new InheritedField;
				worksheet->table->fields.push_back(inheritedField);
			}
			break;*/
			
		case Field::SERVICE:
			{
				ServiceField* serviceField = new ServiceField;
				worksheet->table->fields.push_back(serviceField);
			}
			break;
			
		case Field::TEXT:
			{
				Field* textField = new Field(Field::TEXT);
				worksheet->table->fields.push_back(textField);
			}
			break;
			
		case Field::FLOAT:
			{
				Field* floatField = new Field(Field::FLOAT);
				worksheet->table->fields.push_back(floatField);
			}
			break;
			
		case Field::INT:
			{
				Field* intField = new Field(Field::INT);
				worksheet->table->fields.push_back(intField);
			}
			break;
			
		case Field::LINK:
			{
				Field* linkField = new Field(Field::LINK);
				worksheet->table->fields.push_back(linkField);
			}
			break;

		case Field::BOOL:
			{
				Field* boolField = new Field(Field::BOOL);
				worksheet->table->fields.push_back(boolField);
			}
			break;

		case Field::ARRAY:
			{
				Field* arrayField = new Field(Field::ARRAY);
				worksheet->table->fields.push_back(arrayField);
			}
			break;
			
		default:
			MSG(boost::format("E: PROGRAM ERROR: second row and %d (%s) column: field type = %d is undefined. Refer to software supplier.\n") % (columnIndex + 1) % PrintColumn(columnIndex) % parsing->fieldKeywords.Match(messenger, worksheet->table->realName, rowIndex, columnIndex, typeName));
			return false;
		}
	}
	
	
	//commentaries and names:
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < worksheet->columnsCount; columnIndex++)
	{
		if(worksheet->columnToggles.find(columnIndex) == worksheet->columnToggles.end())
		{
			Field* field = worksheet->FieldFromColumn(messenger, columnIndex);
			if(field != NULL)
			{
				//commentaries:
				/*//inherited fields not need to comment:
				if(field->type != Field::INHERITED)*/
				{
					ExcelFormat::BasicExcelCell* commentCell = worksheet->worksheet->Cell(Parsing::ROW_FIELDS_COMMENTS, columnIndex);
					if(commentCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
					{
						MSG(boost::format("W: commentary for column %d (%s) at row %d within table \"%s\" omitted - it is not good.\n") % (columnIndex + 1) % PrintColumn(columnIndex) % (int)Parsing::ROW_FIELDS_COMMENTS % worksheet->table->realName);
					}
					else
					{
						std::string commentary = GetString(commentCell);
						if(commentary.empty())
						{
							MSG(boost::format("W: commentary for column %d (%s) at row %d within table \"%s\" is not of literal type. It will be skipped.\n") % (columnIndex + 1) % PrintColumn(columnIndex) % (int)Parsing::ROW_FIELDS_COMMENTS % worksheet->table->realName);
						}
						else
						{
							field->commentary = commentary;
						}
					}
				}
				
				//name:
				ExcelFormat::BasicExcelCell* nameCell = worksheet->worksheet->Cell(Parsing::ROW_FIELDS_NAMES, columnIndex);
				if(nameCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
				{
					MSG(boost::format("E: name of field at row %d and column %d (%s) within table \"%s\" was NOT cpecified.\n") % (Parsing::ROW_FIELDS_NAMES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
					return false;
				}
				else
				{
					std::string fieldName = GetString(nameCell);
					if(fieldName.empty())
					{
						MSG(boost::format("E: field's name at row %d and column %d (%s) within table \"%s\" is NOT of literal type.\n") % (Parsing::ROW_FIELDS_NAMES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
						return false;
					}
					else
					{
						//check that such field was not already defined:
						for(size_t i = 0; i < worksheet->table->fields.size(); i++)
						{
							if(worksheet->table->fields[i]->name.compare(fieldName) == 0)
							{
								MSG(boost::format("E: field \"%s\" at row %d and column %d (%s) within table \"%s\" was already defined.\n") % fieldName % (Parsing::ROW_FIELDS_NAMES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
								return false;
							}
						}

						//apply only AFTER duplicates checking:
						field->name = fieldName;
						
						//field type's specific:
						switch(field->type)
						{
						/*case Field::INHERITED:
							{
								Field* parentField = NULL;
								WorksheetTable* root = NULL;
								ForEachTable(worksheet->parent, [field, &root, &parentField](WorksheetTable* parent) -> bool
								{
									for(size_t i = 0; i < parent->table->fields.size(); i++)
									{
										//it's need to find root table where inherited field was firstly defined, so skip intermediate parents where that field was inherited too:
										if(parent->table->fields[i]->type != Field::INHERITED && parent->table->fields[i]->name.compare(field->name) == 0)
										{
											parentField = parent->table->fields[i];
											root = parent;
											return false;
										}
									}
									return true;
								});
								
								if(parentField == NULL)
								{
									MSG(boost::format("E: inherited field with name \"%s\" at column %d within table \"%s\" was NOT declared in any parent.\n") % field->name % (columnIndex + 1) % worksheet->table->realName);
									return false;
								}

								InheritedField* inheritedField = (InheritedField*)field;
								inheritedField->parentField = parentField;
								inheritedField->parent = root->table;
							}
							break;*/

						case Field::SERVICE:
							switch(parsing->serviceFieldsKeywords.Match(messenger, worksheet->table->realName, Parsing::ROW_FIELDS_NAMES, columnIndex, field->name))
							{
							case -1:
								return false;
								break;

							case ServiceField::ID:
								{
									ServiceField* serviceField = (ServiceField*)field;
									serviceField->serviceType = ServiceField::ID;
									//always the same:
									std::transform(serviceField->name.begin(), serviceField->name.end(), serviceField->name.begin(), ::tolower);
								}
								break;
							}
							break;
						}

						//if this field was inherited, wrap it into appropriate container:
						if(worksheet->parent != NULL)
						{
							ForEachTable(worksheet->parent, [worksheet, field](WorksheetTable* parent) -> bool
							{
								for(size_t i = 0; i < parent->table->fields.size(); i++)
								{
									//it's need to find root table where inherited field was firstly defined, so skip intermediate parents where that field was inherited too:
									if(parent->table->fields[i]->type != Field::INHERITED && parent->table->fields[i]->name.compare(field->name) == 0)
									{
										InheritedField* wrapped = new InheritedField;
										wrapped->name = field->name;
										wrapped->commentary = field->commentary;
										wrapped->parentField = parent->table->fields[i];
										wrapped->parent = parent->table;

										//replace previously created with new one:
										for(size_t changingFieldIndex = 0; changingFieldIndex < worksheet->table->fields.size(); changingFieldIndex++)
										{
											if(worksheet->table->fields[changingFieldIndex]->name.compare(field->name) == 0)
											{
												worksheet->table->fields[changingFieldIndex] = wrapped;
												break;
											}
										}

										//old not needed anymore:
										delete field;

										return false;
									}
								}
								return true;
							});
						}
					}
				}
			}
		}
	}
	
	//check that all inherited fields was defined:
	if(worksheet->parent != NULL && worksheet->table->type != Table::VIRTUAL)
	{
		//all inherited fields:
		std::list<DerivedField> derivedFields;
		ForEachTable(worksheet->parent, [&derivedFields](WorksheetTable* parent) -> bool
		{
			for(size_t i = 0; i < parent->table->fields.size(); i++)
			{
				DerivedField derivedField;
				derivedField.field = parent->table->fields[i];
				derivedField.worksheetTable = parent;

				derivedFields.push_back(derivedField);
			}
			return true;
		});
		
		//for each derived field:
		for(std::list<DerivedField>::iterator it = derivedFields.begin(); it != derivedFields.end(); it++)
		{
			bool found = false;
			
			//this table fields:
			for(size_t thisFieldIndex = 0; thisFieldIndex < table->fields.size(); thisFieldIndex++)
			{
				Field* thisField = table->fields[thisFieldIndex];

				if(thisField->type == Field::INHERITED)
				{
					if(it->field->name.compare(thisField->name) == 0)
					{
						InheritedField* inheritedField = (InheritedField*)thisField;

						//check that types are similar:
						if(it->field->type != inheritedField->parentField->type)
						{
							MSG(boost::format("E: derived field \"%s\" within table \"%s\" was defined as \"%s\" within parent table \"%s\" where it is defined as \"%s\". Types must be similar.\n") % thisField->name % worksheet->table->realName % parsing->fieldKeywords.Find(messenger, thisField->type) % it->worksheetTable->table->realName % parsing->fieldKeywords.Find(messenger, it->field->type));
							return false;
						}

						found = true;
						break;
					}
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: derived field \"%s\" was NOT defined within table \"%s\".\n") % (*it).field->name % table->realName);
				return false;
			}
		}
	}
	
	
	worksheet->processed = true;
	return true;
}

#define LINK_FORMAT_ERROR messenger << (boost::format("E: link at row %d and column %d (%s) within table \"%s\" has wrong format: \"%s\"; it has ot be of type \"first_link_table_name:x(y); second_link_table_name:k(l)\", where \"x\" is objects' integral id and \"y\" is integral count; the same for other links separated by semicolons. If link must not be specified so live field just empty.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % text);

/** Returns generated FieldData or NULL on some error.*/
FieldData* ProcessFieldsData(Messenger& messenger, WorksheetTable* worksheet, ExcelFormat::BasicExcelCell* dataCell, Parsing* parsing, Field* field, const int rowIndex, const int columnIndex, std::vector<WorksheetTable*>& worksheets)
{
	switch(field->type)
	{
	case Field::INHERITED:
		{
			InheritedField* inheritedField = (InheritedField*)field;
			FieldData* parent = ProcessFieldsData(messenger, worksheet, dataCell, parsing, inheritedField->parentField, rowIndex, columnIndex, worksheets);
			if(parent == NULL)
			{
				return NULL;
			}
			return new Inherited(inheritedField, rowIndex + 1, columnIndex + 1, parent);
		}
	
	case Field::SERVICE:
		{
			if(dataCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: service field data at row %d and column %d (%s) within table \"%s\" must be defined.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
				return NULL;
			}

			ServiceField* serviceField = (ServiceField*)field;
			switch(serviceField->serviceType)
			{
			case ServiceField::ID:
				{
					Service* service_id = new Service(serviceField, rowIndex + 1, columnIndex + 1);
					try
					{
						service_id->fieldData = new Int(field, rowIndex + 1, columnIndex + 1, GetInt(dataCell));
					}
					catch(...)
					{
						MSG(boost::format("E: service's \"id\" field at row %d and column %d (%s) must have integral (or real-valued, which will be casted to integer) data.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
						return NULL;
					}
					return service_id;
				}
				break;

			default:
				MSG(boost::format("E: PROGRAM ERROR: undefined service field type. Refer to software supplier.\n"));
				return NULL;
			}
		}
		break;
	
	case Field::TEXT:
		{
			std::string text;
			//undefined fields are just empty string:
			if(dataCell->Type() != ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				text = GetString(dataCell);
			}

			return new Text(field, rowIndex + 1, columnIndex + 1, text);
		}
		break;
	
	case Field::FLOAT:
		{
			if(dataCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: real-valued field at row %d and column %d (%s) within table \"%s\" is undefined. It must have real-valued or integral data - type 0 to get rid of this error.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
				return NULL;
			}

			try
			{
				return new Float(field, rowIndex + 1, columnIndex + 1, GetFloat(dataCell));
			}
			catch(...)
			{
				MSG(boost::format("E: real-valued field at row %d and column %d (%s) must have real-valued or integer data (or formula).\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
				return NULL;
			}
		}
		break;
	
	case Field::INT:
		{
			if(dataCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: integral field at row %d and column %d (%s) within table \"%s\" is undefined. It must have integral data - type 0 to get rid of this error.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
				return NULL;
			}
			
			try
			{
				return new Int(field, rowIndex + 1, columnIndex + 1, GetInt(dataCell));
			}
			catch(...)
			{
				MSG(boost::format("E: integral field at row %d and column %d (%s) within table \"%s\" must have integral, real-valued (will be round upped to integral) data or formula.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
				return NULL;
			}
		}
		break;
	
	case Field::LINK:
		{
			std::string text;
			//undefined cell is just "no links":
			if(dataCell->Type() != ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				text = GetString(dataCell);
			}

			Link* link = new Link(field, rowIndex + 1, columnIndex + 1, text);
			
			//boost::split() breaks empty string into array of one length with empty string where my parsing fails:
			if(text.empty() == false)
			{
				std::vector<std::string> links = Parsing::Detach(text, ";");
				for(size_t linkIndex = 0; linkIndex < links.size(); linkIndex++)
				{
					//linked object's count:
					std::vector<std::string> linkAndCount = Parsing::Detach(links[linkIndex], "(");
					int count = 1;
					if(linkAndCount.size() == 2)
					{
						std::vector<std::string> countAndSomething = Parsing::Detach(linkAndCount[1], ")");
						try
						{
							count = boost::lexical_cast<int, std::string>(countAndSomething[0]);
						}
						catch(...)
						{
							LINK_FORMAT_ERROR;
							return NULL;
						}
					
						if(count <= 0)
						{
							MSG(boost::format("E: link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" has count attribute = %d. It has to be at least more than zero.\n") % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % count);
							return NULL;
						}
					}
					else if(linkAndCount.size() > 2)
					{
						LINK_FORMAT_ERROR;
						return NULL;
					}
				
					std::vector<std::string> tableAndId = Parsing::Detach(linkAndCount[0], ":");
					if(tableAndId.size() != 2)
					{
						LINK_FORMAT_ERROR;
						MSG("start\n");
						MSG(boost::format("%s\nend\n") % GetString(dataCell));
						return NULL;
					}
				
					//linked object's id:
					int id = -1;
					try
					{
						id = boost::lexical_cast<int, std::string>(tableAndId[1]);
					}
					catch(...)
					{
						LINK_FORMAT_ERROR;
						return NULL;
					}
				
					//lookup for linked table name:
					bool tableFound = false;
					FOR_EACH_WORKSHEET(worksheetTable)
						if(tableAndId[0].compare(worksheetTable->table->lowercaseName) == 0)
						{
							Table* table = worksheetTable->table;
						
							//it's pointless to link against virtual table:
							if(table->type == Table::VIRTUAL)
							{
								MSG(boost::format("E: link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" links against virtual table \"%s\" - it is pointless to link against virtual tables.\n") % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % table->realName);
								return NULL;
							}
						
							//check that target table has special service table which is obligatory for linkage:
							bool idFieldFound = false;
							ForEachTable(worksheets[worksheetIndex], [&idFieldFound, &table, &parsing, &messenger](WorksheetTable* worksheet) -> bool
							{
								for(size_t targetTableFieldIndex = 0; targetTableFieldIndex < worksheet->table->fields.size(); targetTableFieldIndex++)
								{
									Field* targetTableField = worksheet->table->fields[targetTableFieldIndex];
									if(targetTableField->type == Field::SERVICE)
									{
										ServiceField* serviceField = (ServiceField*)targetTableField;
										switch(serviceField->serviceType)
										{
										case ServiceField::ID:
											idFieldFound = true;
											return false;
										
										//there are no other service fields types but there can be later:
										default:
											return true;
										}
									}
								}
								
								return true;
							});
							if(idFieldFound == false)
							{
								MSG(boost::format("E: link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" links against table \"%s\" which does NOT support it. Target table must have service column \"id\" to support linkage against it.\n") % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % table->realName);
								return NULL;
							}
						
							link->links.push_back(Count(table, id, count));
							tableFound = true;
							break;
						}
					}
					if(tableFound == false)
					{
						MSG(boost::format("E: table with name \"%s\" was NOT found to link against it within link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%d\".\n") % tableAndId[0] % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
						return NULL;
					}
				}
			}
			
			worksheet->links.push_back(link);
			
			return link;
		}
		break;

	case Field::BOOL:
		{
			const char* example = "Type \"true\" or \"1\", \"false\" or \"0\"";

			const std::string value = GetString(dataCell);
			if(value.empty())
			{
				MSG(boost::format("E: boolean field at row %d and column %d (%s) within table \"%s\" is undefined. %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % example);
				return NULL;
			}

			//true:
			if(value.compare("true") == 0 || value.compare("1") == 0)
			{
				return new Bool(field, rowIndex + 1, columnIndex + 1, true);
			}
			else if(value.compare("false") == 0 || value.compare("0") == 0)
			{
				return new Bool(field, rowIndex + 1, columnIndex + 1, false);
			}

			MSG(boost::format("E: boolean field \"%s\" at row %d and column %d (%s) within table \"%s\" has wrong format. %s.\n") % value % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % example);
			return NULL;
		}
		break;

	case Field::ARRAY:
		{
			Array* arrayField = new Array(field, rowIndex + 1, columnIndex + 1);

			const std::string text = GetString(dataCell);

			//empty string is just an empty array...

			std::vector<std::string> values = Parsing::Detach(text, ";");
			for(std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); it++)
			{
				double value;

				try
				{
					value = boost::lexical_cast<double, std::string>(*it);
				}
				catch(...)
				{
					MSG(boost::format("E: array's part \"%s\" (originating from \"%s\") at row %d and column %d (%s) within table \"%s\" is not of numeral type. Type integral or real-valued numerals separated by semicolons.\n") % (*it) % text % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
					return NULL;
				}

				arrayField->values.push_back(value);
			}

			return arrayField;
		}
		break;

	
	default:
		MSG(boost::format("E: PROGRAM ERROR: field's type = %d at row %d and column %d (%s) is undefined. Refer to software supplier.\n") % field->type % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
		return NULL;
	}
}

class DefinedParam
{
public:
	DefinedParam(const int columnIndex, const std::string& param): columnIndex(columnIndex), param(param)
	{
	}

	int columnIndex;
	std::string param;
};

bool Parsing::ProcessXLS(AST& ast, Messenger& messenger, const std::string& fileName)
{
	ExcelFormat::BasicExcel xls;
	if(xls.Load(fileName.c_str()) == false)
	{
		MSG(boost::format("E: \"%s\" was NOT loaded.\n") % fileName);
		return false;
	}
	
	//whole generated code:
	std::vector<WorksheetTable*> worksheets;
	
	const int totalWorkSheets = xls.GetTotalWorkSheets();
	
	std::string xlsName(fileName.c_str(), fileName.size() - 4);
	
	//initializing tables:
	for(int worksheetIndex = 0; worksheetIndex < totalWorkSheets; worksheetIndex++)
	{
		ExcelFormat::BasicExcelWorksheet* worksheet = xls.GetWorksheet(worksheetIndex);
		const char* charWorksheetName = worksheet->GetAnsiSheetName();
		if(charWorksheetName == NULL)
		{
			if(worksheetIndex <= 0)
			{
				MSG(boost::format("E: first (leftmost) spreadsheet's name is NULL. It can happen if non-english symbols used.\n"));
			}
			else if(worksheetIndex < (totalWorkSheets - 1))
			{
				MSG(boost::format("E: spreadsheet's name after \"%s\" is NULL. It can happen if non-english symbols used.\n") % worksheets[worksheetIndex - 1]->name);
			}
			else
			{
				MSG(boost::format("E: last (rightmost) spreadsheet's name (after \"%s\") is NULL. It can happen if non-english symbols used.\n") % worksheets[worksheetIndex - 1]->name);
			}
			
			return false;
		}
		
		std::string worksheetName(charWorksheetName);
		
		if(worksheetName.size() <= 0)
		{
			MSG(boost::format("E: spreadsheet's name after \"%s\" is empty.\n") % worksheets[worksheetIndex - 1]->table->realName);
			return false;
		}
		
		worksheets.push_back(new WorksheetTable(messenger, NULL, worksheet, worksheetName));

		/*std::ostringstream oss;
		worksheets[worksheetIndex]->worksheet->Print(oss);
		MSG(boost::format("I: %s:\n%s\n") % worksheets[worksheetIndex]->name % oss.str());*/
		
		//by designing decision worksheets beginning with exclamation mark have to be evaluated as technical:
		if(worksheetName[0] == '!')
		{
			continue;
		}

		if(worksheets[worksheetIndex]->rowsCount < ROW_FIRST_DATA)
		{
			MSG(boost::format("E: spreadsheet \"%s\" must have at least %d rows but there are just %d.\n") % worksheetName % ROW_FIRST_DATA % worksheets[worksheetIndex]->rowsCount);
			return false;
		}
		if(worksheets[worksheetIndex]->columnsCount <= COLUMN_MIN_COLUMN)
		{
			MSG(boost::format("E: spreadsheet \"%s\" must have at least %d columns but there are just %d.\n") % worksheetName % ((int)COLUMN_MIN_COLUMN + 1) % worksheets[worksheetIndex]->columnsCount);
			return false;
		}
		
		Table* table = new Table(worksheetName);
		worksheets[worksheetIndex]->table = table;
		ast.tables.push_back(table);

		//sheet's commentary:
		{
			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(ROW_TABLE_COMMENTARY, COLUMN_TABLE_VALUE);
			if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("W: spreadsheet's \"%s\" commentary was NOT specified at row %d and column %d (%s). It is not good.\n") % table->realName % (ROW_TABLE_COMMENTARY + 1) % (COLUMN_TABLE_VALUE + 1) % PrintColumn(COLUMN_TABLE_VALUE));
			}
			else
			{
				table->commentary = GetString(cell);
			}
		}
		
		//sheet's type, inheritance and so on:
		{
			std::vector<DefinedParam> definedParams;

			const int rowIndex = ROW_TABLE_TYPE;
			const int columnIndex = COLUMN_TABLE_VALUE;

			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(rowIndex, columnIndex);
			if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: spreadsheet's \"%s\" params at row %d and column %d (%s) is undefined. At least \"type\" param have to be specified.\n") % table->realName % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
				return false;
			}
			
			const std::string pair = GetString(cell);
			if(pair.empty())
			{
				MSG(boost::format("E: spreadsheet's \"%s\" param at row %d and column %d (%s) is NOT literal.\n") % table->realName % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
				return false;
			}
			
			std::vector<std::string> pairs = Detach(pair, ",");
			for(size_t pairIndex = 0; pairIndex < pairs.size(); pairIndex++)
			{
				std::vector<std::string> paramAndValue = Detach(pairs[pairIndex], ":");
				if(paramAndValue.size() != 2)
				{
					MSG(boost::format("E: param \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" is wrong. It has to be of type \"param:value\".\n") % pair % pairs[pairIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName);
					return false;
				}
				
				//check if such param was already defined:
				for(size_t i = 0; i < definedParams.size(); i++)
				{
					if(paramAndValue[0].compare(definedParams[i].param) == 0)
					{
						MSG(boost::format("E: param \"%s\" at row %d and column %d (%s) within worksheet \"%s\" was already defined at row %d and column %d.\n") % paramAndValue[0] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % (definedParams[i].columnIndex + 1));
						return false;
					}
				}
				definedParams.push_back(DefinedParam(columnIndex, paramAndValue[0]));
				
				switch(tableParamsKeywords.Match(messenger, worksheets[worksheetIndex]->table->realName, rowIndex, columnIndex, paramAndValue[0]))
				{
					case -1:
					return false;
					
				case TableParamsKeywords::TYPE:
					{
						const int type = tableTypesKeywords.Match(messenger, worksheets[worksheetIndex]->table->realName, rowIndex, columnIndex, paramAndValue[1]);
						if(type == -1)
						{
							return false;
						}
						table->type = type;
					}
					break;
					
				case TableParamsKeywords::PARENT:
					{
						worksheets[worksheetIndex]->parentName = paramAndValue[1];
						if(worksheets[worksheetIndex]->parentName.compare(table->lowercaseName) == 0)
						{
							MSG(boost::format("E: parent specifier \"%s\" is the same as current spreadsheet name. Spreadsheet cannot be derived from itself.\n") % worksheets[worksheetIndex]->parentName);
							return false;
						}
					}
					break;
				}
			}

			switch(table->type)
			{
			case Table::UNTYPED:
				{
					MSG(boost::format("E: spreadsheet's \"%s\" type was NOT specified at row %d and column %d (%s) - type something like \"type: many\".\n") % table->realName % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
					return false;
				}
				break;

			case Table::PRECISE:
				{
					if(table->parent != NULL)
					{
						MSG(boost::format("E: table \"%s\" of type \"precise\" inherits table \"%s\". Tables of such type cannot inherit other tables.\n") % table->realName % table->parent->realName);
						return false;
					}
				}
				break;
			}
		}

		//column toggles:
		for(int columnIndex = COLUMN_MIN_COLUMN; columnIndex < worksheets[worksheetIndex]->columnsCount; columnIndex++)
		{
			const int rowIndex = ROW_TOGGLES;

			const char* example = "Type 1 if you want this column to be compiled or 0 otherwise";

			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(rowIndex, columnIndex);
			if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: column toggle at row %d and column %d (%s) within table \"%s\" is undefined. %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % example);
				return false;
			}

			try
			{
				const int value = GetInt(cell);

				switch(value)
				{
				case 0:
					worksheets[worksheetIndex]->columnToggles.insert(columnIndex);
					break;

				case 1:
					break;

				default:
					MSG(boost::format("E: column toggle = %d at row %d and column %d (%s) within table \"%s\" has wrong format. %s.\n") % value % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % example);
					return false;
				}
			}
			catch(...)
			{
				MSG(boost::format("E: column toggle at row %d and column %d (%s) within table \"%s\" is NOT of numeral type. %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % example);
				return false;
			}
		}
	}
	
	//connect inherited sheets with each other:
	FOR_EACH_WORKSHEET(worksheet)
		//if worksheet has some parent:
		if(worksheet->parentName.empty() == false)
		{
			bool found = false;
			
			//find worksheet with such name:
			FOR_EACH_WORKSHEET(parentWorksheet)
				//found:
				if(worksheet->parentName.compare(parentWorksheet->table->lowercaseName) == 0)
				{
					worksheet->table->parent = parentWorksheet->table;
					worksheet->parent = parentWorksheet;
					found = true;
					break;
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: parent \"%s\" for spreadsheet \"%s\" does NOT exist.\n") % worksheet->parentName % worksheet->table->realName);
				return false;
			}
		}
	}
	
	//check for possible inheritance recursion:
	FOR_EACH_WORKSHEET(worksheet)
		std::vector<WorksheetTable*> chain;
		if(WatchRecursiveInheritance(messenger, chain, worksheet))
		{
			return false;
		}
	}
	
	//reading fields types:
	//to get types of inherited columns from parents it's need to have such parents to be already processed - so process all tables starting from it's parents:
	FOR_EACH_WORKSHEET(worksheet)
		if(ProcessColumnsTypes(messenger, worksheet, this) == false)
		{
			return false;
		}
	}
	
	//fields data:
	FOR_EACH_WORKSHEET(worksheet)
		//for tables of type "precise":
		//redirection from column specification to column index (constant from Table::Precise::...):
		std::map<int, int> preciseSpecToColumn;
		if(worksheet->table->type == Table::PRECISE || worksheet->table->type == Table::SINGLE)
		{
			std::set<int> definedColumns;

			//look for all needed cells:
			for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < worksheet->columnsCount; columnIndex++)
			{
				//skip commentary columns:
				if(worksheet->columnToggles.find(columnIndex) != worksheet->columnToggles.end())
				{
					continue;
				}

				ExcelFormat::BasicExcelCell* whatCell = worksheet->worksheet->Cell(Parsing::ROW_FIELDS_TYPES, columnIndex);

				const std::string what = GetString(whatCell);
				if(what.empty())
				{
					MSG(boost::format("E: type cell at row %d and column %d (%s) within table \"%s\" is NOT of literal type. It must be one of: %s.\n") % (Parsing::ROW_FIELDS_TYPES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName % preciseTableKeywords.PrintPossible());
					return false;
				}

				const int matched = preciseTableKeywords.Match(messenger, worksheet->table->realName, Parsing::ROW_FIELDS_TYPES, columnIndex, what);

				if(definedColumns.find(matched) != definedColumns.end())
				{
					MSG(boost::format("E: type column \"%s\" at row %d and column %d (%s) within table \"%s\" was already defined.\n") % preciseTableKeywords.Find(messenger, matched) % (Parsing::ROW_FIELDS_TYPES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % worksheet->table->realName);
					return false;
				}

				definedColumns.insert(matched);

				switch(matched)
				{
				case -1:
					return false;

				default:
					preciseSpecToColumn[matched] = columnIndex;
					break;
				}
			}

			//if one of columns was not specified:
			for(std::map<std::string, int>::const_iterator needed = preciseTableKeywords.keywords.begin(); needed != preciseTableKeywords.keywords.end(); needed++)
			{
				bool found = false;

				for(std::set<int>::const_iterator defined = definedColumns.begin(); defined != definedColumns.end(); defined++)
				{
					if((*defined) == needed->second)
					{
						found = true;
						break;
					}
				}

				if(found == false)
				{
					MSG(boost::format("E: column of type \"%s\" within table \"%s\" of type \"%s\" was NOT specified.\n") % needed->first % worksheet->table->realName % tableTypesKeywords.Find(messenger, worksheet->table->type));
					return false;
				}
			}
		}

		for(int rowIndex = ROW_FIRST_DATA; rowIndex < worksheet->rowsCount; rowIndex++)
		{
			//check if this row was toggled off:
			ExcelFormat::BasicExcelCell* toggleCell = worksheet->worksheet->Cell(rowIndex, Parsing::COLUMN_ROWS_TOGGLES);
			if(toggleCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: row's %d toggle at column %d (%s) within table \"%s\" is undefined. Type 1 if you want this row to be compiled or 0 otherwise.\n") % (rowIndex + 1) % (int)Parsing::COLUMN_ROWS_TOGGLES % PrintColumn(Parsing::COLUMN_ROWS_TOGGLES) % worksheet->table->realName);
				return false;
			}
			else if(toggleCell->Type() == ExcelFormat::BasicExcelCell::INT)
			{
				const int value = toggleCell->GetInteger();
				if(value == 0)
				{
					continue;
				}
				else if(value == 1)
				{
				}
				else
				{
					MSG(boost::format("E: row's %d toggle = %d at column %d (%s) within table \"%s\" is wrong. Type 1 if you want this row to be compiled or 0 otherwise.\n") % (rowIndex + 1) % value % (Parsing::COLUMN_ROWS_TOGGLES + 1) % PrintColumn(Parsing::COLUMN_ROWS_TOGGLES) % worksheet->table->realName);
				}
			}

			//virtual tables cannot contain any data:
			if(worksheet->table->type == Table::VIRTUAL)
			{
				MSG(boost::format("E: virtual table \"%s\" contains some data at row %d. Virtual tables cannot contain any data.\n") % worksheet->table->realName % (rowIndex + 1));
				return false;
			}
			
			//relation within "precise" tables is inverted:
			if(worksheet->table->type == Table::PRECISE || worksheet->table->type == Table::SINGLE)
			{
				//for such tables there are always single row:
				worksheet->table->matrix.push_back(std::vector<FieldData*>());

				ExcelFormat::BasicExcelCell* typeCell = worksheet->worksheet->Cell(rowIndex, preciseSpecToColumn[Table::Precise::TYPE]);
				ExcelFormat::BasicExcelCell* nameCell = worksheet->worksheet->Cell(rowIndex, preciseSpecToColumn[Table::Precise::NAME]);
				ExcelFormat::BasicExcelCell* valueCell = worksheet->worksheet->Cell(rowIndex, preciseSpecToColumn[Table::Precise::VALUE]);
				ExcelFormat::BasicExcelCell* commentaryCell = worksheet->worksheet->Cell(rowIndex, preciseSpecToColumn[Table::Precise::COMMENTARY]);

				//field's type:
				Field* field = NULL;
				const std::string typeName = GetString(typeCell);
				if(typeName.empty())
				{
					MSG(boost::format("E: type cell at row %d and column %d (%s) within table \"%s\" must be of literal type.\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::TYPE] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::TYPE]) % worksheet->table->realName);
					return false;
				}
				switch(fieldKeywords.Match(messenger, worksheet->table->realName, rowIndex, preciseSpecToColumn[Table::Precise::TYPE], typeName))
				{
				case -1:
					return false;

				case Field::SERVICE:
					MSG(boost::format("E: cell of type \"service\" at row %d and column %d (%s) within table \"%s\" is wrong for tables of type \"%s\".\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::TYPE] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::TYPE]) % worksheet->table->realName % tableTypesKeywords.Find(messenger, worksheet->table->type));
					return false;
					
				case Field::TEXT:
						field = new Field(Field::TEXT);
					break;
			
				case Field::FLOAT:
						field = new Field(Field::FLOAT);
					break;
			
				case Field::INT:
						field = new Field(Field::INT);
					break;
			
				case Field::LINK:
						field = new Field(Field::LINK);
					break;

				default:
					MSG(boost::format("E: UNDEFINED ERROR: something typed within field of type \"%s\" at row %d and column %d (%s) within table \"%s\" of type \"%s\" is undefined. Refer to software supplier for more info.\n") % preciseTableKeywords.Find(messenger, Table::Precise::TYPE) % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::TYPE] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::TYPE]) % worksheet->table->realName % tableTypesKeywords.Find(messenger, worksheet->table->type));
					return false;
				}

				worksheet->table->fields.push_back(field);

				//field's name:
				field->name = GetString(nameCell);
				if(field->name.empty())
				{
					MSG(boost::format("E: field's name at row %d and column %d (%s) within table \"%s\" is NOT of literal type.\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::NAME] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::NAME]) % worksheet->table->realName);
					return false;
				}

				//field's data:
				FieldData* fieldData = ProcessFieldsData(messenger, worksheet, valueCell, this, field, rowIndex, preciseSpecToColumn[Table::Precise::VALUE], worksheets);
				if(fieldData == NULL)
				{
					return false;
				}

				worksheet->table->matrix[0].push_back(fieldData);

				//field's commentary:
				field->commentary = GetString(commentaryCell);
				if(field->commentary.empty())
				{
					MSG(boost::format("E: field's commentary at row %d and column %d (%s) within table \"%s\" is NOT of literal type.\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::COMMENTARY] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::COMMENTARY]) % worksheet->table->realName);
					return false;
				}
			}
			else
			{
				worksheet->table->matrix.push_back(std::vector<FieldData*>());

				for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < worksheet->columnsCount; columnIndex++)
				{
					Field* field = worksheet->FieldFromColumn(messenger, columnIndex);
					if(field != NULL)
					{
						ExcelFormat::BasicExcelCell* dataCell = worksheet->worksheet->Cell(rowIndex, columnIndex);
					
						FieldData* fieldData = ProcessFieldsData(messenger, worksheet, dataCell, this, field, rowIndex, columnIndex, worksheets);
						if(fieldData == NULL)
						{
							return false;
						}
						worksheet->table->matrix.back().push_back(fieldData);
					}
				}
			}
		}
	}
	
	//check links:
	FOR_EACH_WORKSHEET(worksheet)
		for(size_t linkIndex = 0; linkIndex < worksheet->links.size(); linkIndex++)
		{
			Link* link = worksheet->links[linkIndex];
			for(size_t countIndex = 0; countIndex < link->links.size(); countIndex++)
			{
				Count& count = link->links[countIndex];
				//check that object with such id exists:
				bool found = false;
				for(size_t objectRowIndex = 0; objectRowIndex < count.table->matrix.size(); objectRowIndex++)
				{
					std::vector<FieldData*>& row = count.table->matrix[objectRowIndex];
					for(size_t objectColumnIndex = 0; objectColumnIndex < row.size(); objectColumnIndex++)
					{
						FieldData* fieldData = row[objectColumnIndex];
						Field* field = fieldData->field;
						if(field->type == Field::INHERITED)
						{
							fieldData = ((Inherited*)fieldData)->fieldData;
							field = ((InheritedField*)field)->parentField;
						}

						if(field->type == Field::SERVICE)
						{
							Service* service = (Service*)(fieldData);
							ServiceField* serviceField = (ServiceField*)service->field;
							if(serviceField->serviceType == ServiceField::ID)
							{
								Int* intField = dynamic_cast<Int*>(service->fieldData);
								if(intField != NULL)
								{
									if(intField->value == count.id)
									{
										found = true;
									}
									//only one "id" field per table (one id data per row):
									break;
								}
								else
								{
									MSG(boost::format("E: PROGRAM ERROR: service field \"id\" holds NOT integer data. Refer to software supplier.\n"));
									return false;
								}
							}
						}
					}
					
					if(found)
					{
						break;
					}
				}
				if(found == false)
				{
					MSG(boost::format("E: link \"%s\" at row %d and column %d (%s) within table \"%s\" links against object with id %d within table \"%s\" which does NOT exist.\n") % link->text % link->row % link->column % PrintColumn(link->column - 1) % worksheet->table->realName % count.id % count.table->realName);
					return false;
				}
			}
		}
		
		worksheet->links.clear();
	}
	
	
	
	return true;
}




