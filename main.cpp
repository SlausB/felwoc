
#include "excel_format/ExcelFormat.h"
#include "excel_format/BasicExcel.hpp"
#include "xml/pugixml.hpp"
#include "json/json_spirit.h"

#include "output/messenger.h"
#include "output/outputs/console_output.h"

//configuration file:
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "Windows.h"

ConsoleOutput consoleOutput;
Messenger messenger(&consoleOutput);

#define FAIL(message) messenger << message; errorsCount++; return;
#define MSG(message) messenger << message;


// true - JSON будет печататься с символами отступов:
bool prettyPrint = true;


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

	ExcelFormat::BasicExcel xls;
	if(xls.Load(fileName.c_str()) == false)
	{
		FAIL(boost::format("E: \"%s\" was NOT loaded.\n") % fileName);
	}
	
	const int totalWorkSheets	= xls.GetTotalWorkSheets();

	std::string xlsName(fileName.c_str(), fileName.size() - 4);
	
	//XML:
	pugi::xml_document xml;
	pugi::xml_node documentNode = xml.append_child(xlsName.c_str());
	//JSON:
	json_spirit::Object json;
	
	for(int i = 0; i < totalWorkSheets; i++)
	{
		ExcelFormat::BasicExcelWorksheet* worksheet = xls.GetWorksheet(i);

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

		//XML:
		pugi::xml_node xmlSpreadsheet = documentNode.append_child(worksheetName.c_str());
		//JSON:
		json_spirit::Array jsonSpreadsheet;

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
					ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

					//первый столбец для горизонтальных комментариев:
					if(column < 1)
					{

					}
					else
					{
						if(cell->Type() == ExcelFormat::BasicExcelCell::INT)
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
					ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

					if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
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
					//XML:
					pugi::xml_node xmlItem = xmlSpreadsheet.append_child("item");
					//JSON:
					json_spirit::Object jsonItem;

					//первый столбец зарезервирован для горизонтальных комментариев:
					for(int column = 1; column < columnsCount; column++)
					{
						ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

						//если элемент-столбец текущей строки НЕ является комментарием:
						if(columnsPermissions[column] == true)
						{
							if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
							{
								MSG(boost::format("W: \"%s\": cell at row %d and column %d is undefined! Will be written as is.\n") % worksheetName % (row + 1) % (column + 1));
							}

							//XML:
							pugi::xml_attribute xmlAttribute = xmlItem.append_attribute(attributes[column].c_str());

							switch(cell->Type())
							{
							case ExcelFormat::BasicExcelCell::UNDEFINED:
								{
									//XML:
									xmlAttribute.set_value("__undefined__");
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], "__undefined__"));
								}
								break;
							case ExcelFormat::BasicExcelCell::INT:
								{
									//XML:
									xmlAttribute.set_value(str(boost::format("%d") % cell->GetInteger()).c_str());
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], cell->GetInteger()));
								}
								break;
							case ExcelFormat::BasicExcelCell::DOUBLE:
								{
									std::string doubleAsString = str(boost::format("%f") % cell->GetDouble());
									TruncateValue(doubleAsString);
									
									//XML:
									xmlAttribute.set_value(doubleAsString.c_str());
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], doubleAsString));
								}
								break;
							case ExcelFormat::BasicExcelCell::STRING:
								{
									//XML:
									xmlAttribute.set_value(cell->GetString());
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], cell->GetString()));
								}
								break;
							case ExcelFormat::BasicExcelCell::WSTRING:
								{
									const char* wstringAsChar = ToChar(cell->GetWString());
									//XML:
									xmlAttribute.set_value(wstringAsChar);
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], wstringAsChar));
								}
								break;
							case ExcelFormat::BasicExcelCell::FORMULA:
								{
									const char* formulaAsString = cell->GetString();
									//числовые формулы представлены в виде пустой строки если получать их как строку:
									if(strlen(formulaAsString) <= 0)
									{
										std::string valueAsString = str(boost::format("%f") % cell->GetDouble());
										TruncateValue(valueAsString);
										
										//XML:
										xmlAttribute.set_value(valueAsString.c_str());
										//JSON:
										jsonItem.push_back(json_spirit::Pair(attributes[column], valueAsString));
									}
									//строковые формулы имеют результирующую строку:
									else
									{
										//XML:
										xmlAttribute.set_value(formulaAsString);
										//JSON:
										jsonItem.push_back(json_spirit::Pair(attributes[column], formulaAsString));
									}
								}
								break;
							default:
								{
									//XML:
									xmlAttribute.set_value("__unknown__");
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], "__unknown__"));
								}
								break;
							}
						}
					}

					//JSON:
					jsonSpreadsheet.push_back(jsonItem);
				}
			}
		}

		//JSON:
		json.push_back(json_spirit::Pair(worksheetName, jsonSpreadsheet));
	}

	//запись в XML:
	xml.save_file(str(boost::format("%s.xml") % xlsName).c_str(), PUGIXML_TEXT("\t"), pugi::format_default, pugi::encoding_wchar);
	//запись в JSON:
	std::ofstream os(str(boost::format("%s.json") % xlsName).c_str());
	int jsonSettings = json_spirit::raw_utf8;
	if(prettyPrint)
	{
		jsonSettings |= json_spirit::pretty_print;
	}
	write(json, os, jsonSettings);

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
	//чтение конфигурационного файла:
	boost::property_tree::ptree config;
	try
	{
		boost::property_tree::read_ini("xls2xj.ini", config);

		std::string jsonPrint = config.get<std::string>("json_print", "pretty");
		prettyPrint = jsonPrint.compare("pretty") == 0;
	}
	catch(std::exception& e)
	{
		MSG(boost::format("E: \"xls2xj.ini\" was NOT loaded. Exception: \"%s\". Proceeding with the default settings.\n") % e.what());
	}

	//нужно перейти на итерацию по всем файлам в папке с расширением ".xls"...
	ProcessXLS("design2.xls");

	MSG(boost::format("I: All done. Hit any key to exit...\n"));

	getchar();

	return 0;
}

