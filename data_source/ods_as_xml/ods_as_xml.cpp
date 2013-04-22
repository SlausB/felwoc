
#include "ods_as_xml.h"

#include "zip/minizip/unzip.h"

#include "boost/make_shared.hpp"

#include "../../xml/pugixml.hpp"

#include <stdexcept>

#include <boost/lexical_cast.hpp>


class OdsCell: public Cell
{
public:

	int GetType()
	{
		return type;
	}

	double GetFloat()
	{
		if(type == Cell::FLOAT) return asFloat;
		return 0;
	}
	
	std::string GetString()
	{
		if(type == Cell::STRING) return asString;
		if(type == Cell::FLOAT) return boost::lexical_cast<std::string>(asFloat);
		//undefined:
		return "";
	}

	int type;

	double asFloat;
	std::string asString;

	/** Row and column where cell was first occured during parsing. Needed to properly handle cells which stretched over multiple rows.*/
	int occuredRow;
	int occuredColumn;
	/** 1 for ordinary cells and more for which cells stretched over multiple rows.*/
	int spannedRows;
	int spannedColumns;
	/** 1 for ordinary cells and more for undefined cells in a row.*/
	int repeatedColumns;
};

class OdsSpreadsheet: public Spreadsheet
{
public:

	std::string GetName()
	{
		return name;
	}

	int GetRowsCount()
	{
		return cells.size();
	}

	int GetColumnsCount()
	{
		if(cells.size() > 0)
		{
			return cells[0].size();
		}

		return 0;
	}

	boost::shared_ptr<Cell> GetCell(const int rowIndex, const int columnIndex)
	{
		if(rowIndex < 0 || rowIndex >= cells.size() || columnIndex < 0 || columnIndex >= cells[rowIndex].size())
		{
			throw std::runtime_error("some index out of bounds");
		}

		return cells[rowIndex][columnIndex];
	}

	std::vector<std::vector< boost::shared_ptr<Cell> > > cells;

	std::string name;
};

/** Prolongs cells within upper row which stretched over multiple rows.
\return false on some error */
bool SpanColumn(int& currentColumnIndex, std::vector<boost::shared_ptr<Cell> >& row, const int rowIndex, boost::shared_ptr<OdsSpreadsheet>& addingSpreadsheet, Messenger* messenger)
{
	const int previousRowIndex = rowIndex - 1;
	//current row is first:
	if(previousRowIndex < 0)
	{
		return true;
	}

	if(previousRowIndex != addingSpreadsheet->cells.size() - 2)
	{
		messenger->write(boost::format("E: PROGRAM ERROR: previous row index is wrong. Refer to software supplier.\n"));
		return false;
	}

	std::vector<boost::shared_ptr<Cell> >& previousRow = addingSpreadsheet->cells[previousRowIndex];
	//if all columns ended:
	if(currentColumnIndex >= previousRow.size())
	{
		return true;
	}

	boost::shared_ptr<Cell>& upperCell = previousRow[currentColumnIndex];
	boost::shared_ptr<OdsCell> castedUpperCell = boost::dynamic_pointer_cast<OdsCell, Cell>(upperCell);
	//if upper cell reaches current row stretching down:
	if((castedUpperCell->occuredRow + castedUpperCell->spannedRows) > rowIndex)
	{
		//if upper cell reaches current row stretching to right:
		if((castedUpperCell->occuredColumn + castedUpperCell->spannedColumns) > currentColumnIndex)
		{
			//-1 because ordinary cells have both fields equal to 1:
			for(int i = 0; i < (castedUpperCell->spannedColumns + castedUpperCell->repeatedColumns - 1); i++)
			{
				row.push_back(upperCell);
				currentColumnIndex++;
			}

			//column moved and we can reach now another spanned cell:
			return SpanColumn(currentColumnIndex, row, rowIndex, addingSpreadsheet, messenger);
		}
	}

	return true;
}

#define CHECK_NULL(node) if(node.empty()) { messenger->write(boost::format("E: PROGRAM ERROR: file's \"%s\" format was not evaluated properly. Line %d. Refer to software supplier.\n") % fileName % __LINE__); isOk = false; return; }

OdsAsXml::OdsAsXml(const char* fileName, Messenger* messenger): isOk(true)
{
	//loading file:
	unzFile sourceFile = unzOpen(fileName);
	if(sourceFile == NULL)
	{
		messenger->write( boost::format( "E: source file \"%s\" was NOT opened: it does not exist or invalid).\n" ) % fileName );
		isOk = false;
		return;
	}

	//we need just single file with data:
	if(unzLocateFile(sourceFile, "content.xml", 0) != UNZ_OK)
	{
		messenger->write(boost::format("E: PROGRAM ERROR: OdsAsXml::OdsAsXml(): context file was NOT found within ZIP file \"%s\". Refer to software supplier.\n") % fileName);
		isOk = false;
		return;
	}
	if(unzOpenCurrentFile(sourceFile) != UNZ_OK)
	{
		messenger->write(boost::format("E: PROGRAM ERROR: OdsAsXml::OdsAsXml(): current file was not opened. Refer to software supplier.\n"));
		isOk = false;
		return;
	}

	//loading data from opened ZIP file:
	char* buffer = NULL;
	int currentBufferSize = 0;
	const int BUFFER_SIZE = 8096;
	char tempBuffer[BUFFER_SIZE];
	for(;;)
	{
		const int copied = unzReadCurrentFile(sourceFile, tempBuffer, BUFFER_SIZE);
		if(copied == 0)
		{
			break;
		}
		if(copied < 0)
		{
			messenger->write(boost::format("E: PROGRAM ERROR: OdsAsXml::OdsAsXml(): data reading failed. Refer to software supplier.\n"));
			isOk = false;
			return;
		}

		buffer = (char*)realloc(buffer, currentBufferSize + copied);
		memcpy((void*)(buffer + currentBufferSize), (void*)tempBuffer, copied);
		currentBufferSize += copied;
	}

	//close ZIP file:
	if(unzCloseCurrentFile(sourceFile) != UNZ_OK)
	{
		messenger->write(boost::format("E: PROGRAM ERROR: OdsAsXml::OdsAsXml(): current ZIP file was not closed successfully. Refer to software supplier.\n"));
		isOk = false;
		return;
	}
	if(unzClose(sourceFile) != UNZ_OK)
	{
		messenger->write(boost::format("E: PROGRAM ERROR: OdsAsXml::OdsAsXml(): ZIP file was not closed successfully. Refer to software supplier.\n"));
		isOk = false;
		return;
	}

	//load XML from data:
	pugi::xml_document xmlSource;
	if(xmlSource.load_buffer_inplace_own(buffer, currentBufferSize) == false)
	{
		messenger->write(boost::format("E: PROGRAM ERROR: OdsAsXml::OdsAsXml(): xml loading failed. Refer to software supplier.\n"));
		isOk = false;
		return;
	}
	
	//from XML to objects:
	pugi::xml_node xmlSpreadsheets = xmlSource.document_element().child("office:body").child("office:spreadsheet");
	CHECK_NULL(xmlSpreadsheets);
	for(pugi::xml_node_iterator xmlTableIt = xmlSpreadsheets.begin(); xmlTableIt != xmlSpreadsheets.end(); xmlTableIt++)
	{
		//needed tables are just "table:table", don't know what others are so just skip them:
		if(strcmp(xmlTableIt->name(), "table:table") != 0)
		{
			continue;
		}

		boost::shared_ptr<OdsSpreadsheet> addingSpreadsheet = boost::make_shared<OdsSpreadsheet>();
		
		pugi::xml_attribute xmlTableName = xmlTableIt->attribute("table:name");
		CHECK_NULL(xmlTableName);
		addingSpreadsheet->name = xmlTableName.value();
		if(addingSpreadsheet->name.compare("Constants") == 0)
		{
			//break here...
			printf("");
		}

		//for each row:
		int rowIndex = 0;
		for(pugi::xml_node_iterator xmlRowIt = xmlTableIt->begin(); xmlRowIt != xmlTableIt->end(); xmlRowIt++, rowIndex++)
		{
			if(strcmp(xmlRowIt->name(), "table:table-row") != 0)
			{
				rowIndex--;
				continue;
			}

			addingSpreadsheet->cells.push_back(std::vector<boost::shared_ptr<Cell> >());
			std::vector<boost::shared_ptr<Cell> >& row = addingSpreadsheet->cells.back();

			int currentColumnIndex = 0;
			if(SpanColumn(currentColumnIndex, row, rowIndex, addingSpreadsheet, messenger) == false)
			{
				isOk = false;
				return;
			}

			//for each cell:
			for(pugi::xml_node_iterator xmlCellIt = xmlRowIt->begin(); xmlCellIt != xmlRowIt->end(); xmlCellIt++)
			{
				if(strcmp(xmlCellIt->name(), "table:table-cell") != 0) continue;

				boost::shared_ptr<OdsCell> addingCell = boost::make_shared<OdsCell>();
				addingCell->type = Cell::UNDEFINED;
				addingCell->occuredRow = rowIndex;
				addingCell->occuredColumn = currentColumnIndex;
				addingCell->spannedRows = 1;
				addingCell->spannedColumns = 1;
				addingCell->repeatedColumns = 1;

				pugi::xml_attribute xmlCellType = xmlCellIt->attribute("office:value-type");
				const char* valueName = "office:value";
				if(xmlCellType.empty() == false)
				{
					if(strcmp(xmlCellType.value(), "float") == 0)
					{
						addingCell->type = Cell::FLOAT;

						pugi::xml_attribute xmlFloatValue = xmlCellIt->attribute(valueName);
						CHECK_NULL(xmlFloatValue);
						try
						{
							addingCell->asFloat = boost::lexical_cast<double, const char*>(xmlFloatValue.value());
						}
						catch(...)
						{
							messenger->write(boost::format("E: PROGRAM ERROR: failed cast to float. Refer to software supplier.\n"));
							isOk = false;
							return;
						}
					}
					//here was single type "string" but some other types holds literal data too such as "date" so will not handle all of them separately:
					else
					{
						addingCell->type = Cell::STRING;

						pugi::xml_node xmlText = xmlCellIt->child("text:p").first_child();
						CHECK_NULL(xmlText);
						addingCell->asString = xmlText.value();
					}
				}

				row.push_back(boost::dynamic_pointer_cast<Cell, OdsCell>(addingCell));
				currentColumnIndex++;

				//repeate for all similar (undefined) columns:
				pugi::xml_attribute xmlColumnsRepeating = xmlCellIt->attribute("table:number-columns-repeated");
				if(xmlColumnsRepeating.empty() == false)
				{
					try
					{
						addingCell->repeatedColumns = boost::lexical_cast<int, const char*>(xmlColumnsRepeating.value());
					}
					catch(...)
					{
						messenger->write(boost::format("E: PROGRAM ERROR: columns repeating count value was not casted to int. Refer to software supplier.\n"));
						isOk = false;
						return;
					}
				}

				//fix stretching over multiple rows and columns:
				pugi::xml_attribute xmlRowsSpanned = xmlCellIt->attribute("table:number-rows-spanned");
				if(xmlRowsSpanned.empty() == false)
				{
					try
					{
						addingCell->spannedRows = boost::lexical_cast<int, const char*>(xmlRowsSpanned.value());
					}
					catch(...)
					{
						messenger->write(boost::format("E: PROGRAM ERROR: rows repeating count value was not casted to int. Refer to software supplier.\n"));
						isOk = false;
						return;
					}
				}
				pugi::xml_attribute xmlColumnsSpanned = xmlCellIt->attribute("table:number-columns-spanned");
				if(xmlColumnsSpanned.empty() == false)
				{
					try
					{
						addingCell->spannedColumns = boost::lexical_cast<int, const char*>(xmlColumnsSpanned.value());
					}
					catch(...)
					{
						messenger->write(boost::format("E: PROGRAM ERROR: columns repeating count value was not casted to int. Refer to software supplier.\n"));
						isOk = false;
						return;
					}
				}

				//-2 because cell was already added once previously and both values have 1 at initialization:
				for(int i = 0; i < (addingCell->spannedColumns + addingCell->repeatedColumns - 2); i++)
				{
					row.push_back(boost::dynamic_pointer_cast<Cell, OdsCell>(addingCell));
					currentColumnIndex++;
				}

				if(SpanColumn(currentColumnIndex, row, rowIndex, addingSpreadsheet, messenger) == false)
				{
					isOk = false;
					return;
				}
			}
		}

		//check proper ODS file format understanding - each row must have similar amount of columns:
		for(int rowIndex = 0; rowIndex < addingSpreadsheet->cells.size(); rowIndex++)
		{
			for(int rightRowIndex = 0; rightRowIndex < addingSpreadsheet->cells.size(); rightRowIndex++)
			{
				if(addingSpreadsheet->cells[rowIndex].size() != addingSpreadsheet->cells[rightRowIndex].size())
				{
					messenger->write(boost::format("E: PROGRAM ERROR: rows have different amount of columns. Refer to software supplier.\n"));
					isOk = false;
					return;
				}
			}
		}

		//don't know why but XML has cell fields with attributes such as "<table:table-cell table:number-columns-repeated="1011"/>" so get rid of fully undefined columns:
		//while there are some columns:
		while ( addingSpreadsheet->cells.empty() == false )
		{
			if ( addingSpreadsheet->cells.front().size() > 0 )
			{
				bool allUndefined = true;
				for ( int rowIndex = 0; rowIndex < addingSpreadsheet->cells.size(); ++rowIndex )
				{
					if ( addingSpreadsheet->cells[ rowIndex ].back()->GetType() != Cell::UNDEFINED )
					{
						allUndefined = false;
						break;
					}
				}

				if ( allUndefined )
				{
					for ( int i = 0; i < addingSpreadsheet->cells.size(); ++i )
					{
						addingSpreadsheet->cells[ i ].pop_back();
					}
				}
				else
				{
					break;
				}
			}
			//all cells within all rows are undefines so think that there are no cells at all:
			else
			{
				addingSpreadsheet->cells.clear();
			}
		}
		//the same for rows:
		while ( addingSpreadsheet->cells.empty() == false )
		{
			bool empty = true;
			for ( int columnIndex = 0; columnIndex < addingSpreadsheet->cells.back().size(); ++columnIndex )
			{
				if ( addingSpreadsheet->cells.back()[ columnIndex ]->GetType() != Cell::UNDEFINED )
				{
					empty = false;
					break;
				}
			}

			if ( empty )
			{
				addingSpreadsheet->cells.pop_back();
			}
			else
			{
				break;
			}
		}

		spreadsheets.push_back( boost::dynamic_pointer_cast< Spreadsheet, OdsSpreadsheet >( addingSpreadsheet ) );
	}
}

int OdsAsXml::GetSpreadsheetsCount()
{
	return spreadsheets.size();
}

boost::shared_ptr<Spreadsheet> OdsAsXml::GetSpreadsheet(const int index)
{
	if(index < 0 || index >= spreadsheets.size())
	{
		throw std::runtime_error("spreadsheet index out of range");
	}
	return spreadsheets[index];
}

bool OdsAsXml::IsOk()
{
	return isOk;
}

