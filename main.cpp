
#include "excel_format/ExcelFormat.h"
#include "excel_format/BasicExcel.hpp"
#include "xml/pugixml.hpp"
#include "json/json_spirit.h"

#include "output/messenger.h"
#include "output/outputs/console_output.h"

using namespace ExcelFormat;

#define FAIL(message) messenger << message; errorsCount++; goto END;
#define MSG(message) messenger << message;

void ProcessXLS(const std::string& fileName)
{
	int errorsCount = 0;

	ConsoleOutput consoleOutput;
	Messenger messenger(&consoleOutput);

	BasicExcel xls(fileName.c_str());
	
	const int totalWorkSheets	= xls.GetTotalWorkSheets();
	
	//XML:
	pugi::xml_document xml;
	
	for(int i = 0; i < totalWorkSheets; i++)
	{
		BasicExcelWorksheet* worksheet = xls.GetWorksheet(i);

		std::string worksheetName(worksheet->GetAnsiSheetName());

		const int rowsCount = worksheet->GetTotalRows();
		const int columnsCount = worksheet->GetTotalCols();

		if(rowsCount < 6)
		{
			MSG(boost::format("W: spreadsheet \"%s\" must have at least 6 rows.\n") % worksheetName);
			continue;
		}
		if(columnsCount < 2)
		{
			MSG(boost::format("W: spreadsheet \"%s\" must have at least 2 columns.\n") % worksheetName);
			continue;
		}

		pugi::xml_node spreadsheetNode = xml.append_child(worksheetName.c_str());

		//двумерный массив флагов считываний по вертикали (столбцы):
		std::map<int, bool> columnsPermissions;

		//свойства элементов:
		std::map<int, std::string> attributes;

		for(int row = 0; row < rowsCount; row++)
		{
			//первые четыре строки - это дифференциация пока ненужных flash/server/script...:
			if(row < 4)
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
			else if(row == 4)
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
								attribute.set_value(str(boost::format("%d") % cell->GetInteger()).c_str());
								break;
							case BasicExcelCell::DOUBLE:
								attribute.set_value(str(boost::format("%f") % cell->GetDouble()).c_str());
								break;
							case BasicExcelCell::STRING:
								attribute.set_value(cell->GetString());
								break;
							case BasicExcelCell::WSTRING:
								attribute.set_value("__wchar__");
								break;
							case BasicExcelCell::FORMULA:
								{
									const char* formulaAsString = cell->GetString();
									//числовые формулы представлены в виде пустой строки если получать их как строку:
									if(strlen(formulaAsString) <= 0)
									{
										attribute.set_value(str(boost::format("%f") % cell->GetDouble()).c_str());
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

		/*for(int row = 0; row < rowsCount; row++)
		{
			printf("	");

			for(int column = 0; column < columnsCount; column++)
			{
				BasicExcelCell* cell = worksheet->Cell(row, column);

				switch(cell->Type())
				{
				case BasicExcelCell::UNDEFINED:
					printf("UNDEFINED		");
					break;
				case BasicExcelCell::INT:
					printf("INT: %d		", cell->GetInteger());
					break;
				case BasicExcelCell::DOUBLE:
					printf("DOUBLE: %f		", (float)cell->GetDouble());
					break;
				case BasicExcelCell::STRING:
					printf("STRING: %s		", cell->GetString());
					break;
				case BasicExcelCell::WSTRING:
					printf("WSTRING		");
					break;
				case BasicExcelCell::FORMULA:
					printf("FORMULA: %f		", (float)cell->GetDouble());
					break;
				default:
					printf("UNKNOWN		");
					break;
				}
			}

			printf("\n");
		}*/
	}


	xml.save_file("design.xml");



	/* // create sheet 1 and get the associated BasicExcelWorksheet pointer
	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);
	CellFormat fmt(fmt_mgr);
	BasicExcelCell* cell;


	 // row 1

	fmt.set_format_string(XLS_FORMAT_INTEGER);
	cell = sheet->Cell(0, 0);
	cell->Set(1.);
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_DECIMAL);
	cell = sheet->Cell(0, 1);
	cell->Set(2.);
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_DATE);
	fmt.set_font(ExcelFont().set_weight(FW_BOLD));
	cell = sheet->Cell(0, 2);
	cell->Set("03.03.2000");
	cell->SetFormat(fmt);


	 // row 2

	fmt.set_font(ExcelFont().set_weight(FW_NORMAL));
	fmt.set_format_string(XLS_FORMAT_GENERAL);
	cell = sheet->Cell(1, 0);
	cell->Set("normal");
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_TEXT);
	cell = sheet->Cell(1, 1);
	cell->Set("Text");
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_GENERAL);
	fmt.set_font(ExcelFont().set_weight(FW_BOLD));
	cell = sheet->Cell(1, 2);
	cell->Set("bold");
	cell->SetFormat(fmt);


	xls.SaveAs(path);*/

END:

	if(errorsCount <= 0)
	{
		messenger << boost::format("I: Everything worked successfully.\n");
	}
	else
	{
		messenger << boost::format("E: there was some errors.\n");
	}
}

int main()
{
	ProcessXLS("design2.xls");

	getchar();

	return 0;
}

