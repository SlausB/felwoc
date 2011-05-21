
#include "excel_format/ExcelFormat.h"
#include "excel_format/BasicExcel.hpp"
#include "xml/pugixml.hpp"
#include "json/json_spirit.h"

#include "output/messenger.h"
#include "output/outputs/console_output.h"

#include "Windows.h"

ConsoleOutput consoleOutput;
Messenger messenger(&consoleOutput);

using namespace ExcelFormat;
using namespace json_spirit;

#define FAIL(message) messenger << message; errorsCount++; return;
#define MSG(message) messenger << message;

char CHAR_BUFFER[999999];

char* ToChar(const wchar_t* source)
{
	//const int codePage = CP_ACP;
	const int codePage = CP_UTF8;

	const int result = WideCharToMultiByte(codePage, 0, source, -1, CHAR_BUFFER, sizeof(CHAR_BUFFER), NULL, NULL);
	if(result <= 0)
	{
		MSG(boost::format("E: WideCharToMultiByte() failed.\n"));
		return NULL;
	}
	return CHAR_BUFFER;
}

void TruncateValue(std::string& valueAsString)
{
	//убираем все нули в конце, если число написано в виде десятичной дроби:
	if(valueAsString.find('.') != string::npos)
	{
		while(valueAsString[valueAsString.size() - 1] == '0')
		{
			valueAsString.pop_back();
		}
		if(valueAsString[valueAsString.size() - 1] == '.')
		{
			valueAsString.pop_back();
		}
	}
}

void ProcessXLS(const std::string& fileName)
{
	int errorsCount = 0;

	MSG(boost::format("I: processing \"%s\"...\n") % fileName);

	BasicExcel xls;
	if(xls.Load(fileName.c_str()) == false)
	{
		FAIL(boost::format("E: \"%s\" was NOT loaded.\n") % fileName);
	}
	
	const int totalWorkSheets	= xls.GetTotalWorkSheets();

	std::string xlsName(fileName.c_str(), fileName.size() - 4);
	
	//XML:
	pugi::xml_document xml;
	pugi::xml_node documentNode = xml.append_child(xlsName.c_str());
	
	for(int i = 0; i < totalWorkSheets; i++)
	{
		BasicExcelWorksheet* worksheet = xls.GetWorksheet(i);

		std::string worksheetName(worksheet->GetAnsiSheetName());

		if(worksheetName.size() <= 0)
		{
			continue;
		}

		//по дизайнерскому решению вкладки, начинающиеся с восклицательного знака, должны пропускаться как технические:
		if(worksheetName[0] == '!')
		{
			continue;
		}

		const int rowsCount = worksheet->GetTotalRows();
		const int columnsCount = worksheet->GetTotalCols();

		if(rowsCount < 3)
		{
			MSG(boost::format("W: spreadsheet \"%s\" must have at least 3 rows.\n") % worksheetName);
			continue;
		}
		if(columnsCount < 2)
		{
			MSG(boost::format("W: spreadsheet \"%s\" must have at least 2 columns.\n") % worksheetName);
			continue;
		}

		pugi::xml_node spreadsheetNode = documentNode.append_child(worksheetName.c_str());

		//двумерный массив флагов считываний по вертикали (столбцы):
		std::map<int, bool> columnsPermissions;

		//свойства элементов:
		std::map<int, std::string> attributes;

		for(int row = 0; row < rowsCount; row++)
		{
			//первая строка - определения надобности стобцов:
			if(row < 1)
			{
				for(int column = 0; column < columnsCount; column++)
				{
					BasicExcelCell* cell = worksheet->Cell(row, column);

					//первый столбец для горизонтальных комментариев:
					if(column < 1)
					{

					}
					else
					{
						if(cell->Type() == BasicExcelCell::INT)
						{
							columnsPermissions[column] = cell->GetInteger() != 0;
						}
						else
						{
							columnsPermissions[column] = false;
						}
					}
				}
			}
			//если на очереди строка имён свойств элементов:
			else if(row == 1)
			{
				for(int column = 1; column < columnsCount; column++)
				{
					BasicExcelCell* cell = worksheet->Cell(row, column);

					if(cell->Type() == BasicExcelCell::UNDEFINED)
					{
						continue;
					}

					attributes[column] = cell->GetString();
				}
			}
			else
			{
				//если вся текущая строка НЕ является комментарием:
				if(worksheet->Cell(row, 0)->GetInteger() != 0)
				{
					pugi::xml_node itemNode = spreadsheetNode.append_child("item");

					//первый столбец зарезервирован для горизонтальных комментариев:
					for(int column = 1; column < columnsCount; column++)
					{
						BasicExcelCell* cell = worksheet->Cell(row, column);

						//если элемент-столбец текущей строки НЕ является комментарием:
						if(columnsPermissions[column] == true)
						{
							if(cell->Type() == BasicExcelCell::UNDEFINED)
							{
								FAIL(boost::format("E: cell at row %d and column %d is undefined! Nothing will be generated.\n") % (row + 1) % (column + 1));
							}

							pugi::xml_attribute attribute = itemNode.append_attribute(attributes[column].c_str());

							switch(cell->Type())
							{
							case BasicExcelCell::UNDEFINED:
								attribute.set_value("__undefined__");
								break;
							case BasicExcelCell::INT:
								{
									attribute.set_value(str(boost::format("%d") % cell->GetInteger()).c_str());
								}
								break;
							case BasicExcelCell::DOUBLE:
								{
									std::string doubleAsString = str(boost::format("%f") % cell->GetDouble());
									TruncateValue(doubleAsString);
									attribute.set_value(doubleAsString.c_str());
								}
								break;
							case BasicExcelCell::STRING:
								attribute.set_value(cell->GetString());
								break;
							case BasicExcelCell::WSTRING:
								attribute.set_value(ToChar(cell->GetWString()));
								break;
							case BasicExcelCell::FORMULA:
								{
									const char* formulaAsString = cell->GetString();
									//числовые формулы представлены в виде пустой строки если получать их как строку:
									if(strlen(formulaAsString) <= 0)
									{
										std::string valueAsString = str(boost::format("%f") % cell->GetDouble());
										TruncateValue(valueAsString);
										attribute.set_value(valueAsString.c_str());
									}
									//строковые формулы имеют результирующую строку:
									else
									{
										attribute.set_value(formulaAsString);
									}
								}
								break;
							default:
								MSG("UNKNOWN		");
								attribute.set_value("__unknown__");
								break;
							}
						}
					}
				}
			}
		}
	}

	xml.save_file("design.xml", PUGIXML_TEXT("\t"), pugi::format_default, pugi::encoding_wchar);

END:

	if(errorsCount <= 0)
	{
		MSG(boost::format("I: \"%s\" processed successfully.\n") % fileName);
	}
	else
	{
		MSG(boost::format("E: \"%s\" processed with errors.\n") % fileName);
	}
}

int main()
{
	//нужно перейти на итерацию по всем файлам в папке с расширением ".xls"...
	ProcessXLS("design2.xls");

	MSG(boost::format("I: All done. Hit any key to exit...\n"));

	getchar();

	return 0;
}

