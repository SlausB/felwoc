

#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

class DataSource;
class Spreadsheet;
class Cell;

/** Reference to abstract cell.*/
class Cell
{
friend class Spreadsheet;

public:
	
	/** All possible types of cell.*/
	enum
	{
		UNDEFINED,	/** Cell's data was not specified.*/
		INT,	/** Integral.*/
		FLOAT,	/** Real-valued.*/
		STRING,	/** Literal.*/
	};
	
	/** Returns type of this cell.*/
	int GetType();
	
	/** Returns value if this cell is of type INT. Returns rounded to nearest integer if cell is of type FLOAT. Returns 0 if cell is of types STRING or UNDEFINED.*/
	int GetInt();
	
	/** Returns value is cell is of type INT or FLOAT. Returns 0 is cell is of types STRING or UNDEFINED.*/
	double GetFloat();
	
	/** Returns string of cell. If cell holds INT or FLOAT - returns conversion to string. Returns empty string if cell is of type UNDEFINED.*/
	std::string GetString();

private:
	
	Cell(CellIFace* cellIFace);
	
	CellIFace* cellIFace;
};

/** Reference to abstract table.*/
class Spreadsheet
{
friend class DataSource;

public:
	
	/** Returns maximum rows which spreadsheet uses.*/
	int GetRowsCount();
	/** Returns maximum columns which spreadsheet uses.*/
	int GetColumnsCount();
	
	/** Returns cell by row and column. Throws exception if at least one index is out of bounds.
	If cell was stretched over multiple rows or columns then it will be returned for all coordinates within it's rectangle.*/
	boost::shared_ptr<Cell> GetCell(const int rowIndex, const int columnIndex);


private:
	
	Spreadsheet(SpreadsheetIFace* spreadsheetIFace);
	
	SpreadsheetIFace* spreadsheetIFace;
};

/** Abstract data source as collection of tables.*/
class DataSource
{
public:
	
	virtual int GetSpreadsheetsCount() = 0;
	
	virtual boost::shared_ptr<Spreadsheet> GetSpreadsheet(const int index) = 0;
	
};

#endif//#ifndef DATA_SOURCE_H

