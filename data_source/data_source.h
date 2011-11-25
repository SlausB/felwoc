

#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <string>

#include "boost/shared_ptr.hpp"

//Print():
#include <ostream>


/** Reference to abstract cell. If cell holds formula then GetType returns type to which formula result is evaluated (FLOAT or STRING) and each data obtaining function returns formula result too.*/
class Cell
{
public:
	
	/** All possible types of cell.*/
	enum
	{
		UNDEFINED,	/** Cell's data was not specified.*/
		FLOAT,	/** Real-valued.*/
		STRING,	/** Literal.*/
	};
	
	/** Returns type of this cell.*/
	virtual int GetType() = 0;
	
	/** Returns value if cell is of type FLOAT. Returns 0 is cell is of types STRING or UNDEFINED.*/
	virtual double GetFloat() = 0;
	
	/** Returns string of cell. If cell holds FLOAT - returns conversion to string. Returns empty string if cell is of type UNDEFINED.*/
	virtual std::string GetString() = 0;
};

/** Reference to abstract table.*/
class Spreadsheet
{
public:

	virtual std::string GetName() = 0;
	
	/** Returns maximum rows which spreadsheet uses.*/
	virtual int GetRowsCount() = 0;
	/** Returns maximum columns which spreadsheet uses.*/
	virtual int GetColumnsCount() = 0;
	
	/** Returns cell by row and column. Throws exception if at least one index is out of bounds.
	If cell was stretched over multiple rows or columns then it will be returned for all coordinates within it's rectangle.*/
	virtual boost::shared_ptr<Cell> GetCell(const int rowIndex, const int columnIndex) = 0;
	
	/** Prints table.
	\param output Where to print.
	\param columnsDelimiter Used to separate columns.
	\param rowsDelimiter Used to separate rows spanning it until it will become row length.*/
	void Print(std::ostream* output, const char* columnsDelimiter = "|", const char* rowsDelimiter = "-");
};

/** Abstract data source as collection of tables.*/
class DataSource
{
public:
	
	virtual int GetSpreadsheetsCount() = 0;
	
	virtual boost::shared_ptr<Spreadsheet> GetSpreadsheet(const int index) = 0;

	/** Returnes true if source loading successfully completed.*/
	virtual bool IsOk() = 0;
	
};

#endif//#ifndef DATA_SOURCE_H

