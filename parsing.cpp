

#include "parsing.h"

#include <boost/algorithm/string.hpp>

#include <map>
#include <set>

#include <algorithm>

#include <stdexcept>

#include "ast/ast.h"

#include <boost/program_options/detail/convert.hpp>

#include <boost/lexical_cast.hpp>


int ToInt(const double value)
{
    if(value >= 0.0f) return (int)(value + 0.5f);
    else return (int)(value - 0.5f);
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

int Keywords::Match(Messenger& messenger, const std::string& context, const std::string& tableName, const int rowIndex, const int columnIndex, const std::string& keyword) const
{
	std::map<std::string, int>::const_iterator it = keywords.find(keyword);
	if(it == keywords.end())
	{
		bool notFirst = true;
		MSG(boost::format("E: %s: keyword \"%s\" within table \"%s\", row %d and column %d is undefined. Possible keywords: %s.\n") % context % keyword % tableName % (rowIndex + 1) % (columnIndex + 1) % PrintPossible());
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
	
	WorksheetTable(Table* table, ExcelFormat::BasicExcelWorksheet* worksheet): table(table), worksheet(worksheet), parent(NULL), processed(false)
	{
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
	
	serviceFieldsKeywords.Add("id", ServiceField::ID);

	preciseTableKeywords.Add("type", Table::Precise::TYPE);
	preciseTableKeywords.Add("name", Table::Precise::NAME);
	preciseTableKeywords.Add("value", Table::Precise::VALUE);
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
		if(what[i] != ' ')
		{
			what.erase(0, i);
			break;
		}
	}
	
	for(int i = (int)what.size() - 1; i >= 0; i--)
	{
		if(what[i] != ' ')
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
		//числовые формулы представлены в виде пустой строки если получать их как строку:
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
bool WatchRecursiveInheritance(Messenger& messenger, const std::string& context, std::vector<WorksheetTable*>& chain, WorksheetTable* wt)
{
	for(size_t i = 0; i < chain.size(); i++)
	{
		if(chain[i] == wt)
		{
			MSG(boost::format("E: %s: found recursive inheritance with table \"%s\".\n") % context % wt->table->realName);
			return true;
		}
	}
	
	if(wt->parent == NULL)
	{
		return false;
	}
	chain.push_back(wt);
	return WatchRecursiveInheritance(messenger, context, chain, wt->parent);
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
bool ProcessColumnsTypes(Messenger& messenger, const std::string& context, WorksheetTable* worksheet, Parsing* parsing, const int columnsCount)
{
	//because this function is called from parsing for each table AND it calls himself due to inheritance - it can be called several times for single table:
	if(worksheet->processed)
	{
		return true;
	}

	//if table has some parent then it can be processed only if parent already was, so process it first if it wasn't yet:
	if(worksheet->parent != NULL)
	{
		if(worksheet->parent->processed == false)
		{
			if(ProcessColumnsTypes(messenger, context, worksheet->parent, parsing, columnsCount) == false)
			{
				return false;
			}
		}
	}
	
	Table* table = worksheet->table;
	
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < columnsCount; columnIndex++)
	{
		ExcelFormat::BasicExcelCell* cell = worksheet->worksheet->Cell(Parsing::ROW_FIELDS_TYPES, columnIndex);
		if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
		{
			MSG(boost::format("E: %s: column's type at row %d and column %d within table \"%s\" is undefined.\n") % context % (Parsing::COLUMN_MIN_COLUMN + 1) % (columnIndex + 1) % table->realName);
			return false;
		}
		
		std::string typeName = GetString(cell);
		if(typeName.empty())
		{
			MSG(boost::format("E: %s: cell at row %d and column %d within table \"%s\" is NOT of literal type. It has to be on of the: %s.\n") % context % (Parsing::COLUMN_MIN_COLUMN + 1) % (columnIndex + 1) % table->realName % parsing->fieldKeywords.PrintPossible());
			return false;
		}
		
		switch(parsing->fieldKeywords.Match(messenger, context, worksheet->table->realName, Parsing::ROW_FIELDS_TYPES, columnIndex, typeName))
		{
		case -1:
			return false;
			
		/*case Field::INHERITED:
			{
				if(worksheet->parent == NULL)
				{
					MSG(boost::format("E: %s: table \"%s\" inherits field at column %d but it does NOT have any parent.\n") % context % worksheet->table->realName % (columnIndex + 1));
					return false;
				}

				InheritedField* inheritedField = new InheritedField;
				worksheet->table->fields.push_back(inheritedField);
			}
			break;*/
			
		case Field::SERVICE:
			{
				Field* serviceField = new Field(Field::SERVICE);
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
			
		default:
			MSG(boost::format("E: %s: PROGRAM ERROR: second row and %d column: field type = %d is undefined. Refer to software supplier.\n") % context % (columnIndex + 1) % parsing->fieldKeywords.Match(messenger, context, worksheet->table->realName, Parsing::ROW_FIELDS_TYPES, columnIndex, typeName));
			return false;
		}
	}
	
	
	//commentaries and names:
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < columnsCount; columnIndex++)
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
						MSG(boost::format("W: %s: commentary for column %d within table \"%s\" omitted - it is not good.\n") % context % (columnIndex + 1) % worksheet->table->realName);
					}
					else
					{
						std::string commentary = GetString(commentCell);
						if(commentary.empty())
						{
							MSG(boost::format("W: %s: commentary for column %d within table \"%s\" is not of literal type. It will be skipped.\n") % context % (columnIndex + 1) % worksheet->table->realName);
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
					MSG(boost::format("W: %s: name of field on column %d was NOT cpecified.\n") % context % (columnIndex + 1));
					return false;
				}
				else
				{
					std::string fieldName = GetString(nameCell);
					if(fieldName.empty())
					{
						MSG(boost::format("E: %s: field's name on column %d within table \"%s\" is NOT of literal type.\n") % context % (columnIndex + 1) % worksheet->table->realName);
						return false;
					}
					else
					{
						//check that such field was not already defined:
						for(size_t i = 0; i < worksheet->table->fields.size(); i++)
						{
							if(worksheet->table->fields[i]->name.compare(fieldName) == 0)
							{
								MSG(boost::format("E: %s: field \"%s\" at column %d within table \"%s\" was already defined.\n") % context % fieldName % (columnIndex + 1) % worksheet->table->realName);
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
									MSG(boost::format("E: %s: inherited field with name \"%s\" at column %d within table \"%s\" was NOT declared in any parent.\n") % context % field->name % (columnIndex + 1) % worksheet->table->realName);
									return false;
								}

								InheritedField* inheritedField = (InheritedField*)field;
								inheritedField->parentField = parentField;
								inheritedField->parent = root->table;
							}
							break;*/

						case Field::SERVICE:
							switch(parsing->serviceFieldsKeywords.Match(messenger, context, worksheet->table->realName, Parsing::ROW_FIELDS_NAMES, columnIndex, field->name))
							{
							case -1:
								return false;
								break;
							}
							break;
						}

						//if this field was inherited, wrap it into appropriate container:
						if(worksheet->parent != NULL)
						{
							ForEachTable(worksheet->parent, [&field](WorksheetTable* parent) -> bool
							{
								for(size_t i = 0; i < parent->table->fields.size(); i++)
								{
									//it's need to find root table where inherited field was firstly defined, so skip intermediate parents where that field was inherited too:
									if(parent->table->fields[i]->type != Field::INHERITED && parent->table->fields[i]->name.compare(field->name) == 0)
									{
										delete field;
										InheritedField* wrapped = new InheritedField;
										wrapped->parentField = parent->table->fields[i];
										wrapped->parent = parent->table;
										field = wrapped;
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
		std::set<std::string> parentFields;
		ForEachTable(worksheet->parent, [&parentFields](WorksheetTable* parent) -> bool
		{
			for(size_t i = 0; i < parent->table->fields.size(); i++)
			{
				parentFields.insert(parent->table->fields[i]->name);
			}
			return true;
		});
		
		//this table fields:
		for(std::set<std::string>::iterator it = parentFields.begin(); it != parentFields.end(); it++)
		{
			bool found = false;
			
			for(size_t thisFieldIndex = 0; thisFieldIndex < table->fields.size(); thisFieldIndex++)
			{
				if(it->compare(table->fields[thisFieldIndex]->name) == 0)
				{
					found = true;
					break;
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: %s: derived field \"%s\" was NOT defined within table \"%s\".\n") % context % (*it) % table->realName);
				return false;
			}
		}
	}
	
	
	worksheet->processed = true;
	return true;
}

#define LINK_FORMAT_ERROR messenger << (boost::format("E: %s: link at row %d and column %d within table \"%s\" has wrong format \"%s\"; it has ot be of type \"first_link_table_name:x(y), second_link_table_name:k(l)\", where \"x\" is objects' integral id and \"y\" is integral count; the same for other links separated by commas.\n") % context % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName % text);

/** Returns generated FieldData or NULL on some error.*/
FieldData* ProcessFieldsData(Messenger& messenger, const std::string& context, WorksheetTable* worksheet, ExcelFormat::BasicExcelCell* dataCell, Parsing* parsing, Field* field, const int rowIndex, const int columnIndex, std::vector<WorksheetTable*>& worksheets)
{
	switch(field->type)
	{
	case Field::INHERITED:
		{
			InheritedField* inheritedField = (InheritedField*)field;
			FieldData* parent = ProcessFieldsData(messenger, context, worksheet, dataCell, parsing, inheritedField->parentField, rowIndex, columnIndex, worksheets);
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
				MSG(boost::format("E: service field data at row %d and column %d within table \"%s\" must be defined.\n") % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName);
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
						MSG(boost::format("E: %s: service's \"id\" field at row %d and column %d must have integer (or real-valued, which will be casted to integer) data.\n") % context % (rowIndex + 1) % (columnIndex + 1));
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
				MSG(boost::format("E: real-valued field at row %d and column %d within table \"%s\" is undefined. It must have real-valued or integral data - type 0 to get rid of this error.\n") % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName);
				return NULL;
			}

			try
			{
				return new Float(field, rowIndex + 1, columnIndex + 1, GetFloat(dataCell));
			}
			catch(...)
			{
				MSG(boost::format("E: %s: real-valued field at row %d and column %d must have real-valued or integer data (or formula).\n") % context % (rowIndex + 1) % (columnIndex + 1));
				return NULL;
			}
		}
		break;
	
	case Field::INT:
		{
			if(dataCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: integral field at row %d and column %d within table \"%s\" is undefined. It must have integral data - type 0 to get rid of this error.\n") % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName);
				return NULL;
			}
			
			try
			{
				return new Int(field, rowIndex + 1, columnIndex + 1, GetInt(dataCell));
			}
			catch(...)
			{
				MSG(boost::format("E: %s: integer field at row %d and column %d must have integer, real-valued (will be rounded up to integer) data or formula.\n") % context % (rowIndex + 1) % (columnIndex + 1));
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

			if(text.empty())
			{
				MSG(boost::format("E: %s: link at row %d and column %d within table \"%s\" must contain literal data.\n") % context % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName);
				return NULL;
			}
			else
			{
				Link* link = new Link(field, rowIndex + 1, columnIndex + 1, text);
			
				std::vector<std::string> links = Parsing::Detach(text, ",");
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
							MSG(boost::format("E: %s: link has count attribute = %d. It has to be at least more than zero.\n") % context % count);
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
					for(size_t worksheetIndex = 0; worksheetIndex < worksheets.size(); worksheetIndex++)
					{
						if(tableAndId[0].compare(worksheets[worksheetIndex]->table->lowercaseName) == 0)
						{
							Table* table = worksheets[worksheetIndex]->table;
						
							//it's pointless to link against virtual table:
							if(table->type == Table::VIRTUAL)
							{
								MSG(boost::format("E: %s: link \"%s\" at row %d and column %d within table \"%s\" links against virtual table \"%s\" - it is pointless to link against virtual table.\n") % context % text % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName % table->realName);
								return NULL;
							}
						
							//check that target table has special service table which is obligatory for linkage:
							bool idFieldFound = false;
							ForEachTable(worksheets[worksheetIndex], [&idFieldFound, &table, &parsing, &context, &messenger](WorksheetTable* worksheet) -> bool
							{
								for(size_t targetTableFieldIndex = 0; targetTableFieldIndex < worksheet->table->fields.size(); targetTableFieldIndex++)
								{
									Field* targetTableField = table->fields[targetTableFieldIndex];
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
								MSG(boost::format("E: %s: link \"%s\" at row %d and column %d within table \"%s\" links against table \"%s\" which does NOT support it. Target table must have service column \"id\" to support lingage against it.\n") % context % text % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName % table->realName);
								return NULL;
							}
						
							link->links.push_back(Count(table, id, count));
							tableFound = true;
							break;
						}
					}
					if(tableFound == false)
					{
						MSG(boost::format("E: %s: table with name \"%s\" was NOT found to link against it within link \"%s\" at row %d and column %d within table \"%d\".\n") % context % tableAndId[0] % text % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->realName);
						return NULL;
					}
				}
			
				worksheet->links.push_back(link);
			
				return link;
			}
		}
		break;
	
	default:
		MSG(boost::format("E: %s: PROGRAM ERROR: field's type = %d at row %d and column %d is undefined. Refer to software supplier.\n") % context % field->type % (rowIndex + 1) % (columnIndex + 1));
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
				MSG(boost::format("E: %s: first (leftmost) spreadsheet's name is NULL.\n") % fileName);
			}
			else if(worksheetIndex < (totalWorkSheets - 1))
			{
				MSG(boost::format("E: %s: spreadsheet's name after \"%s\" is NULL.\n") % fileName % worksheets[worksheetIndex - 1]->table->realName);
			}
			else
			{
				MSG(boost::format("E: %s: last (rightmost) spreadsheet's name (after \"%s\") is NULL.\n") % fileName % worksheets[worksheetIndex - 1]->table->realName);
			}
			
			return false;
		}
		
		std::string worksheetName(charWorksheetName);
		
		if(worksheetName.size() <= 0)
		{
			MSG(boost::format("E: %s: spreadsheet's name after \"%s\" is empty.\n") % fileName % worksheets[worksheetIndex - 1]->table->realName);
			return false;
		}
		
		worksheets.push_back(new WorksheetTable(NULL, worksheet));
		
		//by designing decision worksheets beginning with exclamation mark have to be evaluated as technical:
		if(worksheetName[0] == '!')
		{
			continue;
		}

		const int rowsCount = worksheet->GetTotalRows();
		const int columnsCount = worksheet->GetTotalCols();

		if(rowsCount < ROW_FIRST_DATA)
		{
			MSG(boost::format("E: %s: spreadsheet \"%s\" must have at least %d rows.\n") % fileName % worksheetName % ROW_FIRST_DATA);
			return false;
		}
		if(columnsCount <= COLUMN_MIN_COLUMN)
		{
			MSG(boost::format("E: %s: spreadsheet \"%s\" must have at least %d columns.\n") % fileName % worksheetName % ((int)COLUMN_MIN_COLUMN + 1));
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
				MSG(boost::format("W: spreadsheet's \"%s\" commentary was NOT specified at row %d and column %d. It is not good.\n") % table->realName % (ROW_TABLE_COMMENTARY + 1) % (COLUMN_TABLE_VALUE + 1));
			}
			else
			{
				table->commentary = GetString(cell);
			}
		}
		
		//sheet's type, inheritance and so on:
		{
			std::vector<DefinedParam> definedParams;
			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(ROW_TABLE_TYPE, COLUMN_TABLE_VALUE);
			if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: %s: spreadsheet's \"%s\" params at row %d and column %d is undefined. At least \"type\" param have to be specified.\n") % fileName % table->realName % (ROW_TABLE_TYPE + 1) % (COLUMN_TABLE_VALUE + 1));
				return false;
			}
			
			const std::string pair = GetString(cell);
			if(pair.empty())
			{
				MSG(boost::format("E: %s: spreadsheet's \"%s\" param at row %d and column %d is NOT literal.\n") % fileName % table->realName % (ROW_TABLE_TYPE + 1) % (COLUMN_TABLE_VALUE + 1));
				return false;
			}
			
			std::vector<std::string> pairs = Detach(pair, ",");
			for(size_t pairIndex = 0; pairIndex < pairs.size(); pairIndex++)
			{
				std::vector<std::string> paramAndValue = Detach(pairs[pairIndex], ":");
				if(paramAndValue.size() != 2)
				{
					MSG(boost::format("E: %s: param \"%s\" (it's part \"%s\") at column %d within table \"%s\" is wrong. It has to be of type \"param:value\".\n") % fileName % pair % pairs[pairIndex] % (COLUMN_TABLE_VALUE + 1) % table->realName);
					return false;
				}
				
				//check if such param was already defined:
				for(size_t i = 0; i < definedParams.size(); i++)
				{
					if(paramAndValue[0].compare(definedParams[i].param) == 0)
					{
						MSG(boost::format("E: %s: param \"%s\" at first row and column %d within worksheet \"%s\" was already defined at row %d and column %d.\n") % fileName % paramAndValue[0] % (COLUMN_TABLE_VALUE + 1) % table->realName % (definedParams[i].columnIndex + 1));
						return false;
					}
				}
				definedParams.push_back(DefinedParam(COLUMN_TABLE_VALUE, paramAndValue[0]));
				
				switch(tableParamsKeywords.Match(messenger, fileName, worksheets[worksheetIndex]->table->realName, ROW_TABLE_TYPE, COLUMN_TABLE_VALUE, paramAndValue[0]))
				{
					case -1:
					return false;
					
				case TableParamsKeywords::TYPE:
					{
						const int type = tableTypesKeywords.Match(messenger, fileName, worksheets[worksheetIndex]->table->realName, ROW_TABLE_TYPE, COLUMN_TABLE_VALUE, paramAndValue[1]);
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
							MSG(boost::format("E: %s: parent specifier \"%s\" is the same as current spreadsheet name. Spreadsheet cannot be derived from itself.\n") % fileName % worksheets[worksheetIndex]->parentName);
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
					MSG(boost::format("E: spreadsheet's \"%s\" type was NOT specified at row %d and column %d - type something like \"type: many\".\n") % table->realName % (ROW_TABLE_TYPE + 1) % (COLUMN_TABLE_VALUE + 1));
					return false;
				}
				break;

			case Table::PRECISE:
				{
					if(table->parent != NULL)
					{
						MSG(boost::format("E: table \"%s\" of type \"precise\" inherits table \"%s\". Tables of such cannot inherit other tables.\n") % table->realName % table->parent->realName);
						return false;
					}

					if(columnsCount <= COLUMN_PRECISE_VALUE)
					{
						MSG(boost::format("E: spreadsheet \"%s\" with type \"precise\" must have at least %d columns.\n") % table->realName % (COLUMN_PRECISE_VALUE + 1));
						return false;
					}
				}
				break;
			}
		}

		//column toggles:
		for(int columnIndex = COLUMN_MIN_COLUMN; columnIndex < columnsCount; columnIndex++)
		{
			const char* example = "Type 1 if you want this column to be compiled or 0 otherwise";

			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(ROW_TOGGLES, columnIndex);
			if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: column toggle at row %d and column %d within table \"%s\" is undefined. %s.\n") % (ROW_TOGGLES + 1) % (columnIndex + 1) % table->realName % example);
				return false;
			}

			try
			{
				const int value = GetInt(cell);

				switch(value)
				{
				case 0:
					worksheets[worksheetIndex]->columnToggles.insert(value);
					break;

				case 1:
					break;

				default:
					MSG(boost::format("E: column toggle = %d at row %d and column %d within table \"%s\" has wrong format. %s.\n") % value % (ROW_TOGGLES + 1) % (columnIndex + 1) % table->realName % example);
					return false;
				}
			}
			catch(...)
			{
				MSG(boost::format("E: column toggle at row %d and column %d within table \"%s\" is NOT of numeral type. %s.\n") % (ROW_TOGGLES + 1) % (columnIndex + 1) % table->realName % example);
				return false;
			}
		}
	}
	
	//connect inherited sheets with each other:
	for(size_t wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		//if worksheet has some parent:
		if(worksheets[wtIndex]->parentName.empty() == false)
		{
			bool found = false;
			
			//find worksheet with such name:
			for(size_t parentWorksheetIndex = 0; parentWorksheetIndex < worksheets.size(); parentWorksheetIndex++)
			{
				//found:
				if(worksheets[wtIndex]->parentName.compare(worksheets[parentWorksheetIndex]->table->lowercaseName) == 0)
				{
					worksheets[wtIndex]->table->parent = worksheets[parentWorksheetIndex]->table;
					worksheets[wtIndex]->parent = worksheets[parentWorksheetIndex];
					found = true;
					break;
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: %s: parent \"%s\" for spreadsheet \"%s\" does NOT exist.\n") % fileName % worksheets[wtIndex]->parentName % worksheets[wtIndex]->table->realName);
				return false;
			}
		}
	}
	
	//check for possible inheritance recursion:
	for(size_t wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		WorksheetTable* worksheet = worksheets[wtIndex];
		std::vector<WorksheetTable*> chain;
		if(WatchRecursiveInheritance(messenger, fileName, chain, worksheet))
		{
			return false;
		}
	}
	
	//reading fields types:
	//to get types of inherited columns from parents it's need to have such parents to be already processed - so process all tables starting from it's parents:
	for(size_t wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		if(ProcessColumnsTypes(messenger, fileName, worksheets[wtIndex], this, worksheets[wtIndex]->worksheet->GetTotalCols()) == false)
		{
			return false;
		}
	}
	
	//fields data:
	for(size_t wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		const int rowsCount = worksheets[wtIndex]->worksheet->GetTotalRows();
		const int columnsCount = worksheets[wtIndex]->worksheet->GetTotalCols();

		for(int rowIndex = ROW_FIRST_DATA; rowIndex < rowsCount; rowIndex++)
		{
			//check if this row was toggled off:
			ExcelFormat::BasicExcelCell* toggleCell = worksheets[wtIndex]->worksheet->Cell(rowIndex, Parsing::COLUMN_ROWS_TOGGLES);
			if(toggleCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: %s: row's %d toggle within table \"%s\" is undefined. Type 1 if you want this row to be compiled or 0 otherwise.\n") % fileName % (rowIndex + 1) % worksheets[wtIndex]->table->realName);
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
					MSG(boost::format("E: row's %d toggle = %d at column %d within table \"%s\" is wrong. Type 1 if you want this row to be compiled or 0 otherwise.\n") % (rowIndex + 1) % value % (Parsing::COLUMN_ROWS_TOGGLES + 1) % worksheets[wtIndex]->table->realName);
				}
			}

			//virtual tables cannot contain any data:
			if(worksheets[wtIndex]->table->type == Table::VIRTUAL)
			{
				MSG(boost::format("E: virtual table \"%s\" contains some data at row %d. Virtual tables cannot contain any data.\n") % worksheets[wtIndex]->table->realName % (rowIndex + 1));
				return false;
			}
			
			worksheets[wtIndex]->table->matrix.push_back(std::vector<FieldData*>());
			
			for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < columnsCount; columnIndex++)
			{
				Field* field = worksheets[wtIndex]->FieldFromColumn(messenger, columnIndex);
				if(field != NULL)
				{
					ExcelFormat::BasicExcelCell* dataCell = worksheets[wtIndex]->worksheet->Cell(rowIndex, columnIndex);
					
					FieldData* fieldData = ProcessFieldsData(messenger, fileName, worksheets[wtIndex], dataCell, this, field, rowIndex, columnIndex, worksheets);
					if(fieldData == NULL)
					{
						return false;
					}
					worksheets[wtIndex]->table->matrix.back().push_back(fieldData);
				}
			}
		}
	}
	
	//check links:
	for(size_t wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		WorksheetTable* worksheet = worksheets[wtIndex];
		
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
						if(row[objectColumnIndex]->field->type == Field::SERVICE)
						{
							Service* service = (Service*)(row[objectColumnIndex]);
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
									MSG(boost::format("E: %s: PROGRAM ERROR: service field \"id\" holds NOT integer data. Refer to software supplier.\n") % fileName);
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
					MSG(boost::format("E: %s: link \"%s\" links against object with id %d within table \"%s\" which does NOT exist.\n") % fileName % link->text % count.id % count.table->realName);
					return false;
				}
			}
		}
		
		worksheet->links.clear();
	}
	
	
	
	return true;
}




