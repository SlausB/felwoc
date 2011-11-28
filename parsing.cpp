

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



#define FOR_EACH_SPREADSHEET(name) \
	for(size_t spreadsheetIndex = 0; spreadsheetIndex < spreadsheets.size(); spreadsheetIndex++) \
	{ \
		SpreadsheetTable* name = spreadsheets[spreadsheetIndex]; \
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

class SpreadsheetTable
{
public:
	
	SpreadsheetTable(Messenger& messenger, Table* table, boost::shared_ptr<Spreadsheet> spreadsheet): table(table), spreadsheet(spreadsheet), parent(NULL), processed(false)
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
			MSG(boost::format("E: PROGRAM ERROR: resultIndex = %d is more than fields.size() = %u. Refer to software supplier.\n") % resultIndex % table->fields.size());
			return NULL;
		}
		return table->fields[resultIndex];
	}
	
	Table* table;
	
	std::string parentName;
	
	boost::shared_ptr<Spreadsheet> spreadsheet;
	
	SpreadsheetTable* parent;
	
	/** Turned off columns - they must NOT be compiled.*/
	std::set<int> columnToggles;
	
	/** All links collected during data parsing - have to be processed after it.*/
	std::vector<Link*> links;
	
	/** true if table's fields array was filled and it can be used within derived classes.*/
	bool processed;
};

struct DerivedField
{
	Field* field;
	SpreadsheetTable* spreadsheetTable;
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

/** Recursively checks if inheritance has recursion. Returns true at some error.*/
bool WatchRecursiveInheritance(Messenger& messenger, std::vector<SpreadsheetTable*>& chain, SpreadsheetTable* st)
{
	for(size_t i = 0; i < chain.size(); i++)
	{
		if(chain[i] == st)
		{
			MSG(boost::format("E: found recursive inheritance with table \"%s\".\n") % st->table->realName);
			return true;
		}
	}
	
	if(st->parent == NULL)
	{
		return false;
	}
	chain.push_back(st);
	return WatchRecursiveInheritance(messenger, chain, st->parent);
}

/** Calls function for each table's parents including table itself.
\param current Will be passed to acceptor too.
\param acceptor Has to return true if it's need to keep iteration.*/
void ForEachTable(SpreadsheetTable* current, std::function<bool(SpreadsheetTable*)> acceptor)
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
bool ProcessColumnsTypes(Messenger& messenger, SpreadsheetTable* spreadsheet, Parsing* parsing)
{
	//because this function is called from parsing for each table AND it calls himself due to inheritance - it can be called several times for single table:
	if(spreadsheet->processed)
	{
		return true;
	}

	//relation for such tables types is inverted - columns specification is read for each row:
	if(spreadsheet->table->type == Table::PRECISE || spreadsheet->table->type == Table::SINGLE)
	{
		return true;
	}

	//if table has some parent then it can be processed only if parent already was, so process it first if it wasn't yet:
	if(spreadsheet->parent != NULL)
	{
		if(spreadsheet->parent->processed == false)
		{
			if(ProcessColumnsTypes(messenger, spreadsheet->parent, parsing) == false)
			{
				return false;
			}
		}
	}
	
	Table* table = spreadsheet->table;
	
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < spreadsheet->spreadsheet->GetColumnsCount(); columnIndex++)
	{
		//if current column must not be compiled:
		if(spreadsheet->columnToggles.find(columnIndex) != spreadsheet->columnToggles.end())
		{
			continue;
		}

		const int rowIndex = Parsing::ROW_FIELDS_TYPES;

		boost::shared_ptr<Cell> cell = spreadsheet->spreadsheet->GetCell(rowIndex, columnIndex);
		if(cell->GetType() == Cell::UNDEFINED)
		{
			MSG(boost::format("E: column's type at row %d and column %d (%s) within table \"%s\" is undefined.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName);
			return false;
		}
		
		std::string typeName = cell->GetString();
		if(typeName.empty())
		{
			MSG(boost::format("E: cell at row %d and column %d (%s) within table \"%s\" is NOT of literal type. It has to be on of the: %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % parsing->fieldKeywords.PrintPossible());
			return false;
		}
		
		switch(parsing->fieldKeywords.Match(messenger, spreadsheet->table->realName, rowIndex, columnIndex, typeName))
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
				spreadsheet->table->fields.push_back(serviceField);
			}
			break;
			
		case Field::TEXT:
			{
				Field* textField = new Field(Field::TEXT);
				spreadsheet->table->fields.push_back(textField);
			}
			break;
			
		case Field::FLOAT:
			{
				Field* floatField = new Field(Field::FLOAT);
				spreadsheet->table->fields.push_back(floatField);
			}
			break;
			
		case Field::INT:
			{
				Field* intField = new Field(Field::INT);
				spreadsheet->table->fields.push_back(intField);
			}
			break;
			
		case Field::LINK:
			{
				Field* linkField = new Field(Field::LINK);
				spreadsheet->table->fields.push_back(linkField);
			}
			break;

		case Field::BOOL:
			{
				Field* boolField = new Field(Field::BOOL);
				spreadsheet->table->fields.push_back(boolField);
			}
			break;

		case Field::ARRAY:
			{
				Field* arrayField = new Field(Field::ARRAY);
				spreadsheet->table->fields.push_back(arrayField);
			}
			break;
			
		default:
			MSG(boost::format("E: PROGRAM ERROR: second row and %d (%s) column: field type = %d is undefined. Refer to software supplier.\n") % (columnIndex + 1) % PrintColumn(columnIndex) % parsing->fieldKeywords.Match(messenger, spreadsheet->table->realName, rowIndex, columnIndex, typeName));
			return false;
		}
	}
	
	
	//commentaries and names:
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < spreadsheet->spreadsheet->GetColumnsCount(); columnIndex++)
	{
		if(spreadsheet->columnToggles.find(columnIndex) == spreadsheet->columnToggles.end())
		{
			Field* field = spreadsheet->FieldFromColumn(messenger, columnIndex);
			if(field != NULL)
			{
				//commentaries:
				/*//inherited fields not need to comment:
				if(field->type != Field::INHERITED)*/
				{
					boost::shared_ptr<Cell> commentCell = spreadsheet->spreadsheet->GetCell(Parsing::ROW_FIELDS_COMMENTS, columnIndex);
					if(commentCell->GetType() == Cell::UNDEFINED)
					{
						MSG(boost::format("W: commentary for column %d (%s) at row %d within table \"%s\" omitted - it is not good.\n") % (columnIndex + 1) % PrintColumn(columnIndex) % (int)Parsing::ROW_FIELDS_COMMENTS % spreadsheet->table->realName);
					}
					else
					{
						std::string commentary = commentCell->GetString();
						if(commentary.empty())
						{
							MSG(boost::format("W: commentary for column %d (%s) at row %d within table \"%s\" is not of literal type. It will be skipped.\n") % (columnIndex + 1) % PrintColumn(columnIndex) % (int)Parsing::ROW_FIELDS_COMMENTS % spreadsheet->table->realName);
						}
						else
						{
							field->commentary = commentary;
						}
					}
				}
				
				//name:
				boost::shared_ptr<Cell> nameCell = spreadsheet->spreadsheet->GetCell(Parsing::ROW_FIELDS_NAMES, columnIndex);
				if(nameCell->GetType() == Cell::UNDEFINED)
				{
					MSG(boost::format("E: name of field at row %d and column %d (%s) within table \"%s\" was NOT cpecified.\n") % (Parsing::ROW_FIELDS_NAMES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
					return false;
				}
				else
				{
					std::string fieldName = nameCell->GetString();
					if(fieldName.empty())
					{
						MSG(boost::format("E: field's name at row %d and column %d (%s) within table \"%s\" is NOT of literal type.\n") % (Parsing::ROW_FIELDS_NAMES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
						return false;
					}
					else
					{
						//check that such field was not already defined:
						for(size_t i = 0; i < spreadsheet->table->fields.size(); i++)
						{
							if(spreadsheet->table->fields[i]->name.compare(fieldName) == 0)
							{
								MSG(boost::format("E: field \"%s\" at row %d and column %d (%s) within table \"%s\" was already defined.\n") % fieldName % (Parsing::ROW_FIELDS_NAMES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
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
							switch(parsing->serviceFieldsKeywords.Match(messenger, spreadsheet->table->realName, Parsing::ROW_FIELDS_NAMES, columnIndex, field->name))
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
						if(spreadsheet->parent != NULL)
						{
							ForEachTable(spreadsheet->parent, [spreadsheet, field](SpreadsheetTable* parent) -> bool
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
										for(size_t changingFieldIndex = 0; changingFieldIndex < spreadsheet->table->fields.size(); changingFieldIndex++)
										{
											if(spreadsheet->table->fields[changingFieldIndex]->name.compare(field->name) == 0)
											{
												spreadsheet->table->fields[changingFieldIndex] = wrapped;
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
	if(spreadsheet->parent != NULL && spreadsheet->table->type != Table::VIRTUAL)
	{
		//all inherited fields:
		std::list<DerivedField> derivedFields;
		ForEachTable(spreadsheet->parent, [&derivedFields](SpreadsheetTable* parent) -> bool
		{
			for(size_t i = 0; i < parent->table->fields.size(); i++)
			{
				DerivedField derivedField;
				derivedField.field = parent->table->fields[i];
				derivedField.spreadsheetTable = parent;

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
							MSG(boost::format("E: derived field \"%s\" within table \"%s\" was defined as \"%s\" within parent table \"%s\" where it is defined as \"%s\". Types must be similar.\n") % thisField->name % spreadsheet->table->realName % parsing->fieldKeywords.Find(messenger, thisField->type) % it->spreadsheetTable->table->realName % parsing->fieldKeywords.Find(messenger, it->field->type));
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
	
	
	spreadsheet->processed = true;
	return true;
}

#define LINK_FORMAT_ERROR messenger << (boost::format("E: link at row %d and column %d (%s) within table \"%s\" has wrong format: \"%s\"; it has ot be of type \"first_link_table_name:x(y); second_link_table_name:k(l)\", where \"x\" is objects' integral id and \"y\" is integral count; the same for other links separated by semicolons. If link must not be specified so live field just empty.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % text);

/** Returns generated FieldData or NULL on some error.*/
FieldData* ProcessFieldsData(Messenger& messenger, SpreadsheetTable* spreadsheet, boost::shared_ptr<Cell> dataCell, Parsing* parsing, Field* field, const int rowIndex, const int columnIndex, std::vector<SpreadsheetTable*>& spreadsheets)
{
	switch(field->type)
	{
	case Field::INHERITED:
		{
			InheritedField* inheritedField = (InheritedField*)field;
			FieldData* parent = ProcessFieldsData(messenger, spreadsheet, dataCell, parsing, inheritedField->parentField, rowIndex, columnIndex, spreadsheets);
			if(parent == NULL)
			{
				return NULL;
			}
			return new Inherited(inheritedField, rowIndex + 1, columnIndex + 1, parent);
		}
	
	case Field::SERVICE:
		{
			if(dataCell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("E: service field data at row %d and column %d (%s) within table \"%s\" must be defined.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
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
						service_id->fieldData = new Int(field, rowIndex + 1, columnIndex + 1, ToInt(dataCell->GetFloat()));
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
			//undefined fields are just empty string...

			return new Text(field, rowIndex + 1, columnIndex + 1, dataCell->GetString());
		}
		break;
	
	case Field::FLOAT:
		{
			if(dataCell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("E: real-valued field at row %d and column %d (%s) within table \"%s\" is undefined. It must have real-valued or integral data - type 0 to get rid of this error.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
				return NULL;
			}

			try
			{
				return new Float(field, rowIndex + 1, columnIndex + 1, dataCell->GetFloat());
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
			if(dataCell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("E: integral field at row %d and column %d (%s) within table \"%s\" is undefined. It must have integral data - type 0 to get rid of this error.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
				return NULL;
			}
			
			try
			{
				return new Int(field, rowIndex + 1, columnIndex + 1, ToInt(dataCell->GetFloat()));
			}
			catch(...)
			{
				MSG(boost::format("E: integral field at row %d and column %d (%s) within table \"%s\" must have integral, real-valued (will be round upped to integral) data or formula.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
				return NULL;
			}
		}
		break;
	
	case Field::LINK:
		{
			//undefined cell is just "no links"...

			std::string text = dataCell->GetString();

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
							MSG(boost::format("E: link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" has count attribute = %d. It has to be at least more than zero.\n") % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % count);
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
						MSG(boost::format("%s\nend\n") % dataCell->GetString());
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
					FOR_EACH_SPREADSHEET(spreadsheetTable)
						if(tableAndId[0].compare(spreadsheetTable->table->lowercaseName) == 0)
						{
							Table* table = spreadsheetTable->table;
						
							//it's pointless to link against virtual table:
							if(table->type == Table::VIRTUAL)
							{
								MSG(boost::format("E: link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" links against virtual table \"%s\" - it is pointless to link against virtual tables.\n") % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % table->realName);
								return NULL;
							}
						
							//check that target table has special service table which is obligatory for linkage:
							bool idFieldFound = false;
							ForEachTable(spreadsheets[spreadsheetIndex], [&idFieldFound, &table, &parsing, &messenger](SpreadsheetTable* spreadsheet) -> bool
							{
								for(size_t targetTableFieldIndex = 0; targetTableFieldIndex < spreadsheet->table->fields.size(); targetTableFieldIndex++)
								{
									Field* targetTableField = spreadsheet->table->fields[targetTableFieldIndex];
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
								MSG(boost::format("E: link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%s\" links against table \"%s\" which does NOT support it. Target table must have service column \"id\" to support linkage against it.\n") % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % table->realName);
								return NULL;
							}
						
							link->links.push_back(Count(table, id, count));
							tableFound = true;
							break;
						}
					}
					if(tableFound == false)
					{
						MSG(boost::format("E: table with name \"%s\" was NOT found to link against it within link \"%s\" (it's part \"%s\") at row %d and column %d (%s) within table \"%d\".\n") % tableAndId[0] % text % links[linkIndex] % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
						return NULL;
					}
				}
			}
			
			spreadsheet->links.push_back(link);
			
			return link;
		}
		break;

	case Field::BOOL:
		{
			const char* example = "Type \"true\" or \"1\", \"false\" or \"0\"";

			const std::string value = dataCell->GetString();
			if(value.empty())
			{
				MSG(boost::format("E: boolean field at row %d and column %d (%s) within table \"%s\" is undefined. %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % example);
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

			MSG(boost::format("E: boolean field \"%s\" at row %d and column %d (%s) within table \"%s\" has wrong format. %s.\n") % value % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % example);
			return NULL;
		}
		break;

	case Field::ARRAY:
		{
			Array* arrayField = new Array(field, rowIndex + 1, columnIndex + 1);

			const std::string text = dataCell->GetString();

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
					MSG(boost::format("E: array's part \"%s\" (originating from \"%s\") at row %d and column %d (%s) within table \"%s\" is not of numeral type. Type integral or real-valued numerals separated by semicolons.\n") % (*it) % text % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
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

bool Parsing::ProcessSource(AST& ast, Messenger& messenger, DataSource* dataSource)
{
	//whole generated code:
	std::vector<SpreadsheetTable*> spreadsheets;
	
	const int totalSpreadsheets = dataSource->GetSpreadsheetsCount();
	
	//initializing tables:
	for(int spreadsheetIndex = 0; spreadsheetIndex < totalSpreadsheets; spreadsheetIndex++)
	{
		boost::shared_ptr<Spreadsheet> spreadsheet = dataSource->GetSpreadsheet(spreadsheetIndex);
		
		if(spreadsheet->GetName().size() <= 0)
		{
			MSG(boost::format("E: spreadsheet's name after \"%s\" is empty.\n") % spreadsheets[spreadsheetIndex - 1]->table->realName);
			return false;
		}
		
		spreadsheets.push_back(new SpreadsheetTable(messenger, NULL, spreadsheet));
		
		//by designing decision worksheets beginning with exclamation mark have to be evaluated as technical:
		if(spreadsheet->GetName()[0] == '!')
		{
			continue;
		}

		if(spreadsheet->GetRowsCount() < ROW_FIRST_DATA)
		{
			MSG(boost::format("E: spreadsheet \"%s\" must have at least %d rows but there are just %d.\n") % spreadsheet->GetName() % ROW_FIRST_DATA % spreadsheet->GetRowsCount());
			return false;
		}
		if(spreadsheet->GetColumnsCount() < COLUMN_MIN_COLUMN)
		{
			MSG(boost::format("E: spreadsheet \"%s\" must have at least %d columns but there are just %d.\n") % spreadsheet->GetName() % ((int)COLUMN_MIN_COLUMN + 1) % spreadsheet->GetColumnsCount());
			return false;
		}
		
		Table* table = new Table(spreadsheet->GetName());
		spreadsheets[spreadsheetIndex]->table = table;
		ast.tables.push_back(table);

		//sheet's commentary:
		{
			boost::shared_ptr<Cell> cell = spreadsheet->GetCell(ROW_TABLE_COMMENTARY, COLUMN_TABLE_VALUE);
			if(cell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("W: spreadsheet's \"%s\" commentary was NOT specified at row %d and column %d (%s). It is not good.\n") % table->realName % (ROW_TABLE_COMMENTARY + 1) % (COLUMN_TABLE_VALUE + 1) % PrintColumn(COLUMN_TABLE_VALUE));
			}
			else
			{
				table->commentary = cell->GetString();
			}
		}
		
		//sheet's type, inheritance and so on:
		{
			std::vector<DefinedParam> definedParams;

			const int rowIndex = ROW_TABLE_TYPE;
			const int columnIndex = COLUMN_TABLE_VALUE;

			boost::shared_ptr<Cell> cell = spreadsheet->GetCell(rowIndex, columnIndex);
			if(cell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("E: spreadsheet's \"%s\" params at row %d and column %d (%s) is undefined. At least \"type\" param have to be specified.\n") % table->realName % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex));
				return false;
			}
			
			const std::string pair = cell->GetString();
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
				
				switch(tableParamsKeywords.Match(messenger, spreadsheets[spreadsheetIndex]->table->realName, rowIndex, columnIndex, paramAndValue[0]))
				{
					case -1:
					return false;
					
				case TableParamsKeywords::TYPE:
					{
						const int type = tableTypesKeywords.Match(messenger, spreadsheets[spreadsheetIndex]->table->realName, rowIndex, columnIndex, paramAndValue[1]);
						if(type == -1)
						{
							return false;
						}
						table->type = type;
					}
					break;
					
				case TableParamsKeywords::PARENT:
					{
						spreadsheets[spreadsheetIndex]->parentName = paramAndValue[1];
						if(spreadsheets[spreadsheetIndex]->parentName.compare(table->lowercaseName) == 0)
						{
							MSG(boost::format("E: parent specifier \"%s\" is the same as current spreadsheet name. Spreadsheet cannot be derived from itself.\n") % spreadsheets[spreadsheetIndex]->parentName);
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
		for(int columnIndex = COLUMN_MIN_COLUMN; columnIndex < spreadsheets[spreadsheetIndex]->spreadsheet->GetColumnsCount(); columnIndex++)
		{
			const int rowIndex = ROW_TOGGLES;

			const char* example = "Type 1 if you want this column to be compiled or 0 otherwise";

			boost::shared_ptr<Cell> cell = spreadsheet->GetCell(rowIndex, columnIndex);
			if(cell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("E: column toggle at row %d and column %d (%s) within table \"%s\" is undefined. %s.\n") % (rowIndex + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % table->realName % example);
				return false;
			}

			try
			{
				const int value = ToInt(cell->GetFloat());

				switch(value)
				{
				case 0:
					spreadsheets[spreadsheetIndex]->columnToggles.insert(columnIndex);
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
	FOR_EACH_SPREADSHEET(spreadsheet)
		//if worksheet has some parent:
		if(spreadsheet->parentName.empty() == false)
		{
			bool found = false;
			
			//find worksheet with such name:
			FOR_EACH_SPREADSHEET(parentSpreadsheet)
				//found:
				if(spreadsheet->parentName.compare(parentSpreadsheet->table->lowercaseName) == 0)
				{
					spreadsheet->table->parent = parentSpreadsheet->table;
					spreadsheet->parent = parentSpreadsheet;
					found = true;
					break;
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: parent \"%s\" for spreadsheet \"%s\" does NOT exist.\n") % spreadsheet->parentName % spreadsheet->table->realName);
				return false;
			}
		}
	}
	
	//check for possible inheritance recursion:
	FOR_EACH_SPREADSHEET(spreadsheet)
		std::vector<SpreadsheetTable*> chain;
		if(WatchRecursiveInheritance(messenger, chain, spreadsheet))
		{
			return false;
		}
	}
	
	//reading fields types:
	//to get types of inherited columns from parents it's need to have such parents to be already processed - so process all tables starting from it's parents:
	FOR_EACH_SPREADSHEET(spreadsheet)
		if(ProcessColumnsTypes(messenger, spreadsheet, this) == false)
		{
			return false;
		}
	}
	
	//fields data:
	FOR_EACH_SPREADSHEET(spreadsheet)
		//for tables of type "precise":
		//redirection from column specification to column index (constant from Table::Precise::...):
		std::map<int, int> preciseSpecToColumn;
		if(spreadsheet->table->type == Table::PRECISE || spreadsheet->table->type == Table::SINGLE)
		{
			std::set<int> definedColumns;

			//look for all needed cells:
			for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < spreadsheet->spreadsheet->GetColumnsCount(); columnIndex++)
			{
				//skip commentary columns:
				if(spreadsheet->columnToggles.find(columnIndex) != spreadsheet->columnToggles.end())
				{
					continue;
				}

				boost::shared_ptr<Cell> whatCell = spreadsheet->spreadsheet->GetCell(Parsing::ROW_FIELDS_TYPES, columnIndex);

				const std::string what = whatCell->GetString();
				if(what.empty())
				{
					MSG(boost::format("E: type cell at row %d and column %d (%s) within table \"%s\" is NOT of literal type. It must be one of: %s.\n") % (Parsing::ROW_FIELDS_TYPES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName % preciseTableKeywords.PrintPossible());
					return false;
				}

				const int matched = preciseTableKeywords.Match(messenger, spreadsheet->table->realName, Parsing::ROW_FIELDS_TYPES, columnIndex, what);

				if(definedColumns.find(matched) != definedColumns.end())
				{
					MSG(boost::format("E: type column \"%s\" at row %d and column %d (%s) within table \"%s\" was already defined.\n") % preciseTableKeywords.Find(messenger, matched) % (Parsing::ROW_FIELDS_TYPES + 1) % (columnIndex + 1) % PrintColumn(columnIndex) % spreadsheet->table->realName);
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
					MSG(boost::format("E: column of type \"%s\" within table \"%s\" of type \"%s\" was NOT specified.\n") % needed->first % spreadsheet->table->realName % tableTypesKeywords.Find(messenger, spreadsheet->table->type));
					return false;
				}
			}
		}

		for(int rowIndex = ROW_FIRST_DATA; rowIndex < spreadsheet->spreadsheet->GetRowsCount(); rowIndex++)
		{
			//check if this row was toggled off:
			boost::shared_ptr<Cell> toggleCell = spreadsheet->spreadsheet->GetCell(rowIndex, Parsing::COLUMN_ROWS_TOGGLES);
			if(toggleCell->GetType() == Cell::UNDEFINED)
			{
				MSG(boost::format("E: row's %d toggle at column %d (%s) within table \"%s\" is undefined. Type 1 if you want this row to be compiled or 0 otherwise.\n") % (rowIndex + 1) % (int)Parsing::COLUMN_ROWS_TOGGLES % PrintColumn(Parsing::COLUMN_ROWS_TOGGLES) % spreadsheet->table->realName);
				return false;
			}
			else if(toggleCell->GetType() == Cell::FLOAT)
			{
				const int value = ToInt(toggleCell->GetFloat());
				if(value == 0)
				{
					continue;
				}
				else if(value == 1)
				{
				}
				else
				{
					MSG(boost::format("E: row's %d toggle = %d at column %d (%s) within table \"%s\" is wrong. Type 1 if you want this row to be compiled or 0 otherwise.\n") % (rowIndex + 1) % value % (Parsing::COLUMN_ROWS_TOGGLES + 1) % PrintColumn(Parsing::COLUMN_ROWS_TOGGLES) % spreadsheet->table->realName);
				}
			}

			//virtual tables cannot contain any data:
			if(spreadsheet->table->type == Table::VIRTUAL)
			{
				MSG(boost::format("E: virtual table \"%s\" contains some data at row %d. Virtual tables cannot contain any data.\n") % spreadsheet->table->realName % (rowIndex + 1));
				return false;
			}
			
			//relation within "precise" tables is inverted:
			if(spreadsheet->table->type == Table::PRECISE || spreadsheet->table->type == Table::SINGLE)
			{
				//for such tables there are always single row:
				spreadsheet->table->matrix.push_back(std::vector<FieldData*>());

				boost::shared_ptr<Cell> typeCell = spreadsheet->spreadsheet->GetCell(rowIndex, preciseSpecToColumn[Table::Precise::TYPE]);
				boost::shared_ptr<Cell> nameCell = spreadsheet->spreadsheet->GetCell(rowIndex, preciseSpecToColumn[Table::Precise::NAME]);
				boost::shared_ptr<Cell> valueCell = spreadsheet->spreadsheet->GetCell(rowIndex, preciseSpecToColumn[Table::Precise::VALUE]);
				boost::shared_ptr<Cell> commentaryCell = spreadsheet->spreadsheet->GetCell(rowIndex, preciseSpecToColumn[Table::Precise::COMMENTARY]);

				//field's type:
				Field* field = NULL;
				const std::string typeName = typeCell->GetString();
				if(typeName.empty())
				{
					MSG(boost::format("E: type cell at row %d and column %d (%s) within table \"%s\" must be of literal type.\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::TYPE] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::TYPE]) % spreadsheet->table->realName);
					return false;
				}
				switch(fieldKeywords.Match(messenger, spreadsheet->table->realName, rowIndex, preciseSpecToColumn[Table::Precise::TYPE], typeName))
				{
				case -1:
					return false;

				case Field::SERVICE:
					MSG(boost::format("E: cell of type \"service\" at row %d and column %d (%s) within table \"%s\" is wrong for tables of type \"%s\".\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::TYPE] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::TYPE]) % spreadsheet->table->realName % tableTypesKeywords.Find(messenger, spreadsheet->table->type));
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
					MSG(boost::format("E: UNDEFINED ERROR: something typed within field of type \"%s\" at row %d and column %d (%s) within table \"%s\" of type \"%s\" is undefined. Refer to software supplier for more info.\n") % preciseTableKeywords.Find(messenger, Table::Precise::TYPE) % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::TYPE] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::TYPE]) % spreadsheet->table->realName % tableTypesKeywords.Find(messenger, spreadsheet->table->type));
					return false;
				}

				spreadsheet->table->fields.push_back(field);

				//field's name:
				field->name = nameCell->GetString();
				if(field->name.empty())
				{
					MSG(boost::format("E: field's name at row %d and column %d (%s) within table \"%s\" is NOT of literal type.\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::NAME] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::NAME]) % spreadsheet->table->realName);
					return false;
				}

				//field's data:
				FieldData* fieldData = ProcessFieldsData(messenger, spreadsheet, valueCell, this, field, rowIndex, preciseSpecToColumn[Table::Precise::VALUE], spreadsheets);
				if(fieldData == NULL)
				{
					return false;
				}

				spreadsheet->table->matrix[0].push_back(fieldData);

				//field's commentary:
				field->commentary = commentaryCell->GetString();
				if(field->commentary.empty())
				{
					MSG(boost::format("E: field's commentary at row %d and column %d (%s) within table \"%s\" is NOT of literal type.\n") % (rowIndex + 1) % (preciseSpecToColumn[Table::Precise::COMMENTARY] + 1) % PrintColumn(preciseSpecToColumn[Table::Precise::COMMENTARY]) % spreadsheet->table->realName);
					return false;
				}
			}
			else
			{
				spreadsheet->table->matrix.push_back(std::vector<FieldData*>());

				for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < spreadsheet->spreadsheet->GetRowsCount(); columnIndex++)
				{
					Field* field = spreadsheet->FieldFromColumn(messenger, columnIndex);
					if(field != NULL)
					{
						boost::shared_ptr<Cell> dataCell = spreadsheet->spreadsheet->GetCell(rowIndex, columnIndex);
					
						FieldData* fieldData = ProcessFieldsData(messenger, spreadsheet, dataCell, this, field, rowIndex, columnIndex, spreadsheets);
						if(fieldData == NULL)
						{
							return false;
						}
						spreadsheet->table->matrix.back().push_back(fieldData);
					}
				}
			}
		}
	}
	
	//check links:
	FOR_EACH_SPREADSHEET(spreadsheet)
		for(size_t linkIndex = 0; linkIndex < spreadsheet->links.size(); linkIndex++)
		{
			Link* link = spreadsheet->links[linkIndex];
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
					MSG(boost::format("E: link \"%s\" at row %d and column %d (%s) within table \"%s\" links against object with id %d within table \"%s\" which does NOT exist.\n") % link->text % link->row % link->column % PrintColumn(link->column - 1) % spreadsheet->table->realName % count.id % count.table->realName);
					return false;
				}
			}
		}
		
		spreadsheet->links.clear();
	}
	
	
	
	return true;
}




