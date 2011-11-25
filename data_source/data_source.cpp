
#include "data_source.h"

#include <vector>

std::string GetString(boost::shared_ptr<Cell> cell)
{
	switch(cell->GetType())
	{
	case Cell::UNDEFINED:
		return "undefined";

	default:
		return cell->GetString();
	}
}

void Spreadsheet::Print(std::ostream* output, const char* columnsDelimiter, const char* rowsDelimiter)
{
	//count maximum length for each cell:
	std::vector<int> cellLengths;
	for(size_t rowIndex = 0; rowIndex < GetRowsCount(); rowIndex++)
	{
		for(size_t columnIndex = 0; columnIndex < GetColumnsCount(); columnIndex++)
		{
			if(cellLengths.size() <= columnIndex)
			{
				cellLengths.push_back(0);
			}

			const int length = GetString(GetCell(rowIndex, columnIndex)).size();

			if(length > cellLengths[columnIndex])
			{
				cellLengths[columnIndex] = length;
			}
		}
	}

	const size_t columnsDelimiterLength = strlen(columnsDelimiter);
	size_t wholeLength = 0;
	for(size_t i = 0; i < cellLengths.size(); i++)
	{
		//+1 for prefix space, +1 for postfix:
		wholeLength += cellLengths[i] + 2 + columnsDelimiterLength;
	}
	//at left on whole table and at right and -1 for unneded last delimiter:
	wholeLength += columnsDelimiterLength * 2 - 1;
	const size_t rowsDelimiterLength = strlen(rowsDelimiter);
	const size_t rowsDelimiterRepeatingCount = wholeLength / rowsDelimiterLength;

	//finally print:
	//upper frame:
	for(size_t i = 0; i < rowsDelimiterRepeatingCount; i++)
	{
		output->write(rowsDelimiter, rowsDelimiterLength);
	}
	output->put('\n');
	//rows:
	for(size_t rowIndex = 0; rowIndex < GetRowsCount(); rowIndex++)
	{
		//cells (columns):
		for(size_t columnIndex = 0; columnIndex < GetColumnsCount(); columnIndex++)
		{
			//left frame:
			if(columnIndex == 0)
			{
				output->write(columnsDelimiter, columnsDelimiterLength);
			}
			output->put(' ');

			const std::string value = GetString(GetCell(rowIndex, columnIndex));
			const int length = value.size();

			const int skip = cellLengths[columnIndex] - length;

			const int skipBefore = (int)((float)skip / 2.0);
			for(int spaces = 0; spaces < skipBefore; spaces++)
			{
				output->put(' ');
			}

			output->write(value.c_str(), value.size());

			const int skipAfter = skip - skipBefore;
			for(int spaces = 0; spaces < skipAfter; spaces++)
			{
				output->put(' ');
			}

			//right frame:
			output->put(' ');
			output->write(columnsDelimiter, columnsDelimiterLength);
		}
		output->put('\n');

		//lower frame:
		for(size_t i = 0; i < rowsDelimiterRepeatingCount; i++)
		{
			output->write(rowsDelimiter, rowsDelimiterLength);
		}
		output->put('\n');
	}
}

