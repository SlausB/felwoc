

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
	
	Table* table;
	
	std::string parentName;
	
	ExcelFormat::BasicExcelWorksheet* worksheet;
	
	ExcelFormat::BasicExcelWorksheet* parent;
	
	/** true if table's fields array was field and it can be used within derived classes.*/
	bool fieldsFilled;
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
const char* GetString(ExcelFormat::BasicExcelCell* cell)
{
	if(cell->Type() == ExcelFormat::BasicExcelCell::STRING)
	{
		return cell->GetString();
	}
	else if(cell ->Type() == ExcelFormat::BasicExcelCell::WSTRING)
	{
		return ToChar(cell->GetWString());
	}
	else
	{
		return NULL;
	}
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

/** Recursively parses columns types from parents tables to children.
\return false on some error.*/
bool ProcessTablesColumns(Messenger& messenger, const std::string& context, WorksheetTable* worksheet)
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
	
	for(int columnIndex = 1; columnIndex < columnsCount; columnIndex++)
	{
		ExcelFormat::BasicExcelCell* cell = worksheet->Cell(1, column);
		if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
		{
			MSG(boost::format("E: %s: column's type cell at second row and column %d is undefined.\n") % context % (columnIndex + 1));
			return false;
		}
		
		const char* typeName = GetString(cell);
		if(typeName == NULL)
		{
			MSG(boost::format("E: %s: cell at second row and column %d is NOT of literal type. It has to be on of the: %s.\n") % context, fieldKeywords.PrintPossible());
			return false;
		}
		
		switch(fieldKeywords.Match(context, typeName))
		{
			case -1:
			return false;
			
			case Field::INHERITED:
			
			break;
			
			case Field::SERVICE:
			break;
			
			case Field::TEXT:
			break;
			
			case Field::FLOAT:
			break;
			
			case Field::INT:
			break;
			
			case Field::LINK:
			break;
			
			default:
			MSG(boost::format("E: %s: PROGRAM ERROR: second row and %d column: field type = %d is undefined. Refer to software supplier.\n") % context % (columnIndex + 1) % fieldKeywords.Match(fileName, typeName));
			return false;
		}
	}
	
	worksheet->processed = true;
	return true;
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

		const int MIN_ROWS = 4;
		if(rowsCount < MIN_ROWS)
		{
			MSG(boost::format("E: %s: spreadsheet \"%s\" must have at least %d rows.\n") % fileName % worksheetName % MIN_ROWS);
			return false;
		}
		const int MIN_COLUMNS = 2;
		if(columnsCount < MIN_COLUMNS)
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
			ExcelFormat::BasicExcelCell* cell = worksheet->Cell(0, column);
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
		if(ProcessTablesColumns(messenger, fileName, worksheets[wtIndex]) == false)
		{
			return false;
		}
	}
	
	
	
	return true;
}




