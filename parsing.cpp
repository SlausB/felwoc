

#include "parsing.h"

#include <boost/algorithm/string.hpp>

#include <map>

#include <algorithm>


int Keywords::Match(const std::string& context, const std::string& keyword) const
{
	std::map<std::string, int>::iterator it = keywords.find(keyword);
	if(it == keywords.end())
	{
		bool notFirst = true;
		MSG(boost::format("E: %s: keyword \"%s\" is undefined. Possible keywords: %s.\n") % context % keyword % PrintPossible());
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

std::string Keywords::PrintPossible()
{
	std::string result;
	for(std::map<std::string, int>::iterator it = keywords.begin(); it != keywords.end(); it++)
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
	
	WorksheetTable(Table* table, ExcelFormat::BasicExcelWorksheet* worksheet): table(table), worksheet(worksheet), parent(NULL), fieldsFilled(false)
	{
	}
	
	/** Returns field accordingly toggled columns or NULL if column index is wrong or it was skipped.*/
	Field* FieldFromColumn(const int columnIndex)
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
		
		const int resultIndex = columnIndex - skipped - COLUMN_MIN_COLUMN;
		if(resultIndex < 0)
		{
			MSG(boost::format("E: PROGRAM ERROR: resultIndex = %d is wrong. Refer to software supplier.\n") % resultIndex);
			return NULL;
		}
		if(resultIndex >= table->fields.size())
		{
			MSG(boost::format("E: PROGRAM ERROR: resultIndex = %d is more that fields.size() = %u. Refer to software supplier.\n") % resultIndex % table->fields.size());
			return NULL;
		}
		return table->fields[resultIndex];
	}
	
	Table* table;
	
	std::string parentName;
	
	ExcelFormat::BasicExcelWorksheet* worksheet;
	
	ExcelFormat::BasicExcelWorksheet* parent;
	
	/** true if table's fields array was field and it can be used within derived classes.*/
	bool fieldsFilled;
	
	/** Turned off columns - they must NOT be compiled.*/
	std::set<int> columnToggles;
	
	/** All links collected during data parsing - have to be processed after it.*/
	std::vector<Link*> links;
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
	
	fieldKeywords.Add("inherited", Field::INHERITED);
	fieldKeywords.Add("service", Field::SERVICE);
	fieldKeywords.Add("text", Field::TEXT);
	fieldKeywords.Add("float", Field::FLOAT);
	fieldKeywords.Add("int", Field::INT);
	fieldKeywords.Add("link", Field::LINK);
	fieldKeywords.Add("null", Field::FIELD_NULL);
	
	serviceFieldsKeywords.Add("id", Service::ID);
}

static std::vector<std::string> Parsing::Detach(const std::string what, const char* delimiters)
{
	std::vector<std::string> strs;
	boost::split(strs, what, boost::is_any_of(delimiters));
	for(int i = 0; i < strs.size(); i++)
	{
		Cleanup(strs[i]);
		std::transform(strs[i].begin(), strs[i].end(), strs[i].begin(), ::tolower);
	}
	return strs;
}

static Parsing::Cleanup(std::string& what)
{
	for(int i = 0; i < what.size(); i++)
	{
		if(what[i] != ' ')
		{
			what.erase(0, i);
			break;
		}
	}
	
	int postfix = 0;
	for(int i = what.size() - 1; i >= 0; i--)
	{
		if(what[i] != ' ')
		{
			what.erase(i, std::string::npos);
			break;
		}
	}
}

/** Returns string if cell is of type STRING of WSTRING, otherwise returns NULL.*/
std::string GetString(ExcelFormat::BasicExcelCell* cell)
{
	if(cell->Type() == ExcelFormat::BasicExcelCell::STRING)
	{
		return cell->GetString();
	}
	else if(cell ->Type() == ExcelFormat::BasicExcelCell::WSTRING)
	{
		return ToChar(cell->GetWString());
	}
	
	return std::string();
}

/** Recursively checks if inheritance has recursion. Returns true at some error.*/
bool WatchRecursiveInheritance(Messenger& messenger, const std::string& context, std::vector<WorksheetTable*>& chain, WorksheetTable* wt)
{
	for(int i = 0; i < chain.size(); i++)
	{
		if(chain[i] == wt)
		{
			MSG(boost::format("E: %s: found recursion inheritance with table \"%s\".\n") % context % wt->table->name);
			return true;
		}
	}
	
	if(wt->parent == NULL)
	{
		return false;
	}
	chain.push_back(wt);
	return WatchRecursiveInheritance(context, chain, wt->parent);
}

/** Calls function for each table's parents including table itself.
\param current Will be passed to acceptor too.
\param acceptor Has to return true if it's need to keep iteration.*/
void ForEachTable(WorksheetTable* current, bool(acceptor*)(WorksheetTable*))
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
bool ProcessTablesColumnsTypes(Messenger& messenger, const std::string& context, WorksheetTable* worksheet, Parsing* parsing, const int columnsCount)
{
	//if table has some parent then it can be processed only if parent already was, so process it first if it wasn't yet:
	if(worksheet->parent != NULL)
	{
		if(worksheet->parent->processed == false)
		{
			if(ProcessTablesColumns(messenger, context, worksheet->parent) == false)
			{
				return false;
			}
		}
	}
	
	Table* table = worksheet->table;
	
	for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < columnsCount; columnIndex++)
	{
		ExcelFormat::BasicExcelCell* cell = worksheet->Cell(Parsing::ROW_FIELDS_TYPES, column);
		if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
		{
			MSG(boost::format("E: %s: column's type cell at second row and column %d is undefined.\n") % context % (columnIndex + 1));
			return false;
		}
		
		std::string typeName = GetString(cell);
		if(typeName.empty())
		{
			MSG(boost::format("E: %s: cell at second row and column %d is NOT of literal type. It has to be on of the: %s.\n") % context, fieldKeywords.PrintPossible());
			return false;
		}
		
		switch(fieldKeywords.Match(context, typeName))
		{
		case -1:
			return false;
			
		case Field::INHERITED:
			{
			if(worksheet->parent == NULL)
			{
				return false;
			}
			Field* parentField = NULL;
			Worksheet* root = NULL;
			ForEachTable(worksheet->parent, [&parentField](WorksheetTable* parent) -> bool
			{
				for(int i = 0; i < parent->table->fields.size(); i++)
				{
					if(parent->table->fields[i].compare(typeName) == 0 && parent->table->fields[i].type != Field::INHERITED)
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
				MSG(boost::format("E: %s: inherited field at column %d within table \"%s\" was NOT declared in any parent.\n") % context % (columnIndex + 1) % worksheet->table->name);
				return false;
			}
			
			InheritedField* inheritedField = new InheritedField;
			inheritedField->type = Field::INHERITED;
			inheritedField->parent = root->table;
			worksheet->table->fields.push_back(inheritedField);
			break;
			
		case Field::SERVICE:
			Field* serviceField = new Field;
			serviceField->type = Field::SERVICE;
			worksheet->table->fields.push_back(serviceField);
			break;
			
		case Field::TEXT:
			Field* textField = new Field;
			textField->type = Field::TEXT;
			worksheet->table->fields.push_back(textField);
			break;
			
		case Field::FLOAT:
			Field* floatField = new Field;
			floatField->type = Field::FLOAT;
			worksheet->table->fields.push_back(floatField);
			break;
			
		case Field::INT:
			Field* intField = new Field;
			intField->type = Field::INT;
			worksheet->table->fields.push_back(intField);
			break;
			
		case Field::LINK:
			Field* linkField = new Field;
			linkField->type = FIELD::LINK;
			worksheet->table->fields.push_back(linkField);
			break;
			
		case Field::FIELD_NULL:
			worksheet->columnToggles.insert(columnIndex);
			break;
			
		default:
			MSG(boost::format("E: %s: PROGRAM ERROR: second row and %d column: field type = %d is undefined. Refer to software supplier.\n") % context % (columnIndex + 1) % fieldKeywords.Match(fileName, typeName));
			return false;
		}
	}
	
	//check that all inherited fields was defined:
	if(worksheet->parent != NULL && worksheet->table->type != Table::VIRTUAL)
	{
		std::set<std::string> parentFields;
		ForEachTable(worksheet->parent, [&parentFields](WorksheetTable* parent) -> bool
		{
			for(int i = 0; i < parent->table->fields.size(); i++)
			{
				parentFields.insert(parent->table->fields[i]);
			}
			return true;
		});
		
		
		for(int parentFieldIndex = 0; parentFieldIndex < parentFields.size(); parentFieldIndex++)
		{
			bool found = false;
			
			for(int thisFieldIndex = 0; thisFieldIndex < table->fields.size(); thisFieldIndex++)
			{
				if(parentFields[parentFieldIndex].compare(table->fields[thisFieldIndex]) == 0)
				{
					found = true;
					break;
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: %s: derived field \"%s\" was NOT defined within table \"%s\".\n") % context % parent->fields[parentFieldIndex].name % table->name);
				return false;
			}
		}
	}
	
	
	//commentaries and names:
	if(worksheet->columnTogles.find(columnIndex) != worksheet->columnToggles.end())
	{
		Field* field = worksheet->FieldFromColumn(columnIndex);
		if(field != NULL)
		{
			//commentaries:
			ExcelFormat::BasicExcelCell* commentCell = worksheet->Cell(Parsing::ROW_FIELDS_COMMENTS, columnIndex);
			if(commentCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("W: %s: commentary for column %d ommited - it's not good.\n") % context % (columnIndex + 1));
			}
			else
			{
				std::string commentary = GetString(commentCell);
				if(commentary.empty())
				{
					MSG(boost::format("W: %s: commentary for column %d is not of literal type. It will be skipped.\n") % context % (columnIndex + 1));
				}
				else
				{
					field->commentary = commentary;
				}
			}
			
			//name:
			ExcelFormat::BasicExcelCell* nameCell = worksheet->Cell(Parsing::ROW_FIELDS_NAMES, columnIndex);
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
					MSG(boost::format("W: %s: field's name on column %d is NOT of literal type.\n") % context % (columnIndex + 1));
					return false;
				}
				else
				{
					field->name = fieldName;
					
					//check that such field was not already defined:
					for(int i = 0; i < worksheet->table->fields.size(); i++)
					{
						if(worksheet->table->fields[i].compare(fieldName) == 0)
						{
							MSG(boost::format("W: %s: field \"%s\" at column %d was already defined.\n") % context % fieldName % (columnIndex + 1));
							return false;
						}
					}
					
					switch(field->type)
					{
					case Field::SERVICE:
						switch(parsing->serviceFieldsKeywords.Match(context, field->name))
						{
						case -1:
							return false;
							break;
						}
						break;
					}
				}
			}
		}
	}
	
	
	worksheet->processed = true;
	return true;
}

#define LINK_FORMAT_ERROR messenger << (boost::format("E: %s: link at row %d and column %d within table \"%s\" has wrong format \"%s\"; it has ot be of type \"first_link_table_name:x(y), second_link_table_name:k(l)\", where \"x\" is objects' integral id and \"y\" is integral count; the same for other links separated by commas.\n") % context % (rowIndex + 1) % (columnIndex + 1) worksheet->table->name % text);

/** Returns generated FieldData or NULL on some error.*/
FieldData* ProcessFieldsData(Messenger& messenger, const std::string& context, WorksheetTable* worksheet, ExcelFormat::BasicExcelCell* dataCell, Parsing* parsing, const int rowIndex, const int columnIndex)
{
	if(dataCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
	{
		MSG(boost::format("E: %s: data cell at row %d and column %d is undefined.\n") % context % (rowIndex + 1) % (columnIndex + 1));
		return NULL;
	}
	
	switch(type)
	{
	case Field::INHERITED:
		FieldData* parent = ProcessFieldsData(messenger, context, field->field, rowIndex, columnIndex);
		if(parentField == NULL)
		{
			return NULL;
		}
		FieldData* inherited = new Inherited(field);
		inherited->fieldData = parent;
		return inherited;
	
	case Field::SERVICE:
		switch(parsing->serviceFieldsKeywords->Match(context, field->name))
		{
		case -1:
			return NULL;
		
		case Service::ID:
			if(dataCell->Type() == ExcelFormat::BasicExcelCell::INT)
			{
				return (new Int(field, dataCell->GetInteger()));
			}
			else
			{
				MSG(boost::format("E: %s: service's \"id\" field at row %d and column %d must have integer data.\n") % context % (rowIndex + 1) % (columnIndex + 1));
				return NULL;
			}
			break;
		}
		break;
	
	case Field::TEXT:
		const std::string text = GetString(dataCell);
		if(text.empty())
		{
			MSG(boost::format("E: %s: literal field at row %d and column %d must have literal data.\n") % context % (rowIndex + 1) % (columnindex + 1));
			return NULL;
		}
		else
		{
			return new Text(field, text);
		}
		break;
	
	case Field::FLOAT:
		if(dataCell->Type() == ExcelFormat::BasicExcelCell::DOUBLE)
		{
			return new Float(field, dataCell->GetDouble());
		}
		else if(dataCell->Type() == ExcelFormat::BasicExcelCell::INT)
		{
			return new Float(field, dataCell->GetInteger());
		}
		else
		{
			MSG(boost::format("E: %s: real-valued field at row %d and column %d must have real-valued or integer data.\n") % context % (rowIndex + 1) % (columnIndex + 1));
			return NULL;
		}
		break;
	
	case Field::INT:
		if(dataCell->Type() == ExcelFormat::BasicExcelCell::INT)
		{
			return new Int(field, dataCell->GetInteger());
		}
		else
		{
			MSG(boost::format("E: %s: integer field at row %d and column %d must have integer data.\n") % context % (rowIndex + 1) % (columnIndex + 1));
			return NULL;
		}
		break;
	
	case Field::LINK:
		const std::string text = GetString(dataCell);
		if(text.empty())
		{
			MSG(boost::format("E: %s: link at row %d and column %d has to contain literal data.\n") % context % (rowIndex + 1) % (columnindex + 1));
			return NULL;
		}
		else
		{
			Link* link = new Link(field);
			
			std::vector<std::string> links = Parsing::Detach(text, ",");
			for(int linkIndex = 0; linkIndex < links.size(); linkIndex++)
			{
				//linked object's count:
				std::vector<std::string> linkAndCount = Parsing::Detach(links[linkIndex], "(");
				int count = 1;
				if(linkAndCount.size() == 2)
				{
					std::vector<std::string> countAndSomething = Parsing::Detach(linkAndCount[1], ")");
					try
					{
						count = boost::lexical_cast<int, std::string>(coundAndSomething[0]);
					}
					catch
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
					id = boost::lexical_cast<std::string, int>(tableAndId[1]);
				}
				catch
				{
					LINK_FORMAT_ERROR;
					return NULL;
				}
				
				//lookup for linked table name:
				bool tableFound = false;
				for(int worksheetIndex = 0; worksheetIndex < worksheets.size(); worksheetIndex++)
				{
					if(tableAndId[0].compare(worksheets[worksheetIndex]->table->name) == 0)
					{
						Table* table = worksheets[worksheetIndex]->table;
						
						//check that target table has special service table which is obligatory for linkage:
						/// DON'T FORGET TO PROPERLY HANDLE INHERITANCE!
						bool idFieldFound = false;
						for(int targetTableFieldIndex = 0; targetTableFieldIndex < table->fields.size(); targetTableFieldIndex++)
						{
							Field* targetTableField = table->fields[targetTableFieldIndex];
							if(targetTableField->type == Field::SERVICE)
							{
								switch(parsing.serviceFieldsKeywords.Match(context, targetTableField->name))
								{
								case -1:
									return NULL;
								
								case Service::ID:
									idFieldFound = true;
									break;
								}
							}
							
							if(idFieldFound)
							{
								break;
							}
						}
						if(idFieldFound == false)
						{
							MSG(boost::format("E: %s: ....
							return NULL;
						}
						
						link->links.push_back(Count(table, id, count));
						tableFound = true;
						break;
					}
				}
				if(tableFound == false)
				{
					MSG(boost::format("E: %s: table with name \"%s\" was NOT found to link against within link \"%s\" at row %d and column %d within table \"%d\".\n") % context % text % (rowIndex + 1) % (columnIndex + 1) % worksheet->table->name);
					return NULL;
				}
			}
			
			worksheet->links.push_back(link);
			
			return link;
		}
		break;
	
	default:
		MSG(boost::format("E: %s: PROGRAM ERROR: field's type = %d at row %d and column %d is undefined. Refer to software supplier.\n") % context % field->type % (rowIndex + 1) % (columnIndex + 1));
		return NULL;
	}
}


bool Parsing::ProcessXLS(Messenger& messenger, const std::string& fileName)
{
	ExcelFormat::BasicExcel xls;
	if(xls.Load(fileName.c_str()) == false)
	{
		FAIL(boost::format("E: \"%s\" was NOT loaded.\n") % fileName);
	}
	
	//whole generated code:
	std::vector<WorksheetTable> worksheets;
	AST ast;
	
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
				MSG(boost::format("E: %s: spreadsheet's name after \"%s\" is NULL.\n") % fileName % worksheets[worksheetIndex - 1].table->name);
			}
			else
			{
				MSG(boost::format("E: %s: last (rightmost) spreadsheet's name (after \"%s\") is NULL.\n") % fileName % worksheets[worksheetIndex - 1].table->name);
			}
			
			return false;
		}
		
		std::string worksheetName(charWorksheetName);
		
		if(worksheetName.size() <= 0)
		{
			MSG(boost::format("E: %s: spreadsheet's name after \"%s\" is empty.\n") % fileName % worksheets[worksheetIndex - 1].table->name);
			return false;
		}
		
		worksheets.push_back(WorksheetTable(NULL, worksheet);
		
		//by designing decision sheets beginning with exclamation mark have to be evaluated as technical:
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
			MSG(boost::format("E: %s: spreadsheet \"%s\" must have at least %d columns.\n") % fileName % worksheetName % MIN_COLUMNS);
			return false;
		}
		
		Table* table = new Table(worksheetName);
		worksheets[worksheetIndex].table = table;
		ast.tables.push_back(table);
		
		//first row - sheet's type, inheritance and so on:
		std::vector<std::string> definedParams;
		for(int column = 0; column < columnsCount; column++)
		{
			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(ROW_TABLE_TYPE, column);
			if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				continue;
			}
			
			const char* pair = GetString(cell);
			if(pair == NULL)
			{
				MSG(boost::format("E: %s: spreadsheet's param (first row) at column %d is NOT literal.\n") % fileName % (column + 1));
				return false;
			}
			
			std::vector<std::string> pairs = Detach(pair, ",");
			for(int pairsIndex = 0; pairsIndex < pairs.size(); pairsIndex++)
			{
				std::vector<std::string> paramAndValue = Detach(pairs[pairsIndex], ";");
				if(paramAndValue.size() != 2)
				{
					MSG(boost::format("E: %s: param \"%s\" (it's part \"%s\") at column %d is wrong. It has to be of type \"param:value\".\n") % fileName % pair % pairs[pairsIndex] % (column + 1));
					return false;
				}
				
				//check if such param was already defined:
				for(int i = 0; i < definedParams.size(); i++)
				{
					if(paramAndValue[0].compare(definedParams[i]) == 0)
					{
						MSG(boost::format("E: %s: param \"%s\" was already defined.\n") % fileName % paramAndValue[0]);
						return false;
					}
				}
				
				switch(tableParamsKeywords.Match(paramAndValue[0]))
				{
					case -1:
					return false;
					
					case TableKeywords::TYPE:
					const int type = tableTypesKeywords.Match(paramAndValue[1]);
					if(type == -1)
					{
						return false;
					}
					table->type = type;
					break;
					
					case TableKeywords::PARENT:
					table->parent = paramAndValue[1];
					if(table->parent.compare(table->name) == 0)
					{
						MSG(boost::format("E: %s: parent specifier \"%s\" is the same as current spreadsheet name. Spreadsheet cannot be derived from itself.\n") % fileName % table->parent);
						return false;
					}
					break;
				}
			}
		}
	}
	
	//connect inherited sheets with each other:
	for(int wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		//if worksheet has some parent:
		if(worksheets[wtIndex].parent.empty() == false)
		{
			bool found = false;
			
			//find worksheet with such name:
			for(int parentWorksheetIndex = 0; parentWorksheetIndex < worksheets.size(); parentWorksheetIndex++)
			{
				//found:
				if(worksheets[wtIndex].table->name.compare(worksheets[parentWorksheetIndex].table->name) == 0)
				{
					worksheets[wtIndex].table->parent = worksheets[parentWorksheetIndex].table;
					worksheets[wtIndex].parent = worksheets[parentWorksheetIndex];
					found = true;
					break;
				}
			}
			
			if(found == false)
			{
				MSG(boost::format("E: %s: parent \"%s\" for spreadsheet \"%s\" does NOT exist.\n") % fileName % worksheets[wtIndex].parent % worksheets[wtIndex].table->name);
				return false;
			}
		}
	}
	
	//check for possible inheritance recursion:
	for(int wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		WorksheetTable* worksheet = worksheets[wtIndex];
		if(worksheet->parent != NULL)
		{
			std::vector<WorksheetTable*> chain;
			chain.push_back(worksheet);
			if(WatchRecursiveInheritance(messenger, fileName, chain, worksheet->parent))
			{
				return false;
			}
		}
	}
	
	//reading fields types (first column is for vertical toggles):
	//to get types of inherited columns from parents it's need to have such parents to be already processed - so process all tables starting from it's parents:
	for(int wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		if(ProcessTablesColumns(messenger, fileName, worksheets[wtIndex], this, columnsCount) == false)
		{
			return false;
		}
	}
	
	//fields data:
	for(int wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		for(int rowIndex = ROW_FIRST_DATA; rowIndex < rowsCount; rowsCount++)
		{
			//check if this row was toggled off:
			ExcelFormat::BasicExcelCell* toggleCell = worksheet->Cell(rowIndex, Parsing::COLUMN_ROWS_TOGGLES);
			if(toggleCell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
			{
				MSG(boost::format("E: %s: row's %d toggle is undefined. Type 1 if you want this row to be compiled or 0 otherwise.\n") % context % (rowIndex + 1));
				return false;
			}
			else if(toggleCell->Type() == ExcelFormat::BasicExcelCell::INT)
			{
				const int value = cell->GetInteger();
				if(value == 0)
				{
					continue;
				}
				else if(value == 1)
				{
				}
				else
				{
					MSG(boost::format("E: %s: row's %d toggle = %d is wrong. Type 1 if you want this row to be compiled or 0 otherwise.\n") % context % (rowIndex + 1));
				}
			}
			
			worksheet->table->matrix.push_back(std::vector<FieldData*>());
			
			for(int columnIndex = Parsing::COLUMN_MIN_COLUMN; columnIndex < columnsCount; columnIndex++)
			{
				Field* field = worksheet->FrieldFromColumn(columnIndex);
				if(field != NULL)
				{
					ExcelFormat::BasicExcelCell* dataCell = worksheet->Cell(rowIndex, columnIndex);
					
					FieldData* fieldData = ProcessFieldsData(messenger, fileName, dataCell, field);
					if(fieldData == NULL)
					{
						return false;
					}
					worksheet->table->matrix.back().push_back(fieldData);
				}
			}
		}
	}
	
	//check links:
	for(int wtIndex = 0; wtIndex < worksheets.size(); wtIndex++)
	{
		WorksheetTable* worksheet = worksheets[wtIndex];
		
		for(int linkIndex = 0; linkIndex = worksheet->links.size(); linkIndex++)
		{
			
		}
	}
	
	
	
	return true;
}




