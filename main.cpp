
#include "excel_format/ExcelFormat.h"
#include "excel_format/BasicExcel.hpp"
#include "xml/pugixml.hpp"
#include "json/json_spirit.h"

#include "output/messenger.h"
#include "output/outputs/console_output.h"
#include "output/outputs/file_output.h"

//configuration file:
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <boost/filesystem.hpp>

#include "Windows.h"

ConsoleOutput consoleOutput;
Messenger messenger(&consoleOutput);


// true - JSON будет печататься с символами отступов:
bool prettyPrint = true;


char CHAR_BUFFER[999999];

char* UNDEFINED_NAME = "__undefined_name__";
char* UNDEFINED_VALUE = "__undefined_value__";
char* NULL_NAME = "__null_name__";
char* UNKNOWN_VALUE = "__unknown_value__";

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
			valueAsString.resize(valueAsString.size() - 1);
		}
		if(valueAsString[valueAsString.size() - 1] == '.')
		{
			valueAsString.resize(valueAsString.size() - 1);
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
		const char* charWorksheetName = worksheet->GetAnsiSheetName();
		if(charWorksheetName == NULL)
		{
			MSG(boost::format("W: NULL spreadsheet name encountered.\n"));
			continue;
		}

		MSG(boost::format("I: spreadsheet \"%s\": processing...\n") % charWorksheetName);

		std::string worksheetName(charWorksheetName);

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
			//первая строка - определения надобности столбцов:
			if(row < 1)
			{
				for(int column = 0; column < columnsCount; column++)
				{
					ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

					//первый столбец для горизонтальных тумблер:
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
					//если столбец является комментарием, то с ним ничего делать не нужно - он никогда не должен использоваться:
					if(columnsPermissions[column] == false)
					{
						continue;
					}

					ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

					if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
					{
						MSG(boost::format("E: spreadsheet \"%s\": item name at %d column is undefined. Applying \"%s\".\n") % worksheetName % (column + 1) % UNDEFINED_NAME);
						attributes[column] = UNDEFINED_NAME;
					}
					else
					{
						const char* cellString = cell->GetString();
						if(cellString == NULL)
						{
							MSG(boost::format("E: spreadsheet \"%s\": item name at %d column is null. Applying \"%s\".\n") % worksheetName % (column + 1) % NULL_NAME);
							attributes[column] = NULL_NAME;
						}
						else
						{
							attributes[column] = cellString;
						}
					}
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

					//первый столбец зарезервирован для горизонтальных тумблеров:
					for(int column = 1; column < columnsCount; column++)
					{
						ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

						//если элемент-столбец текущей строки НЕ является комментарием:
						if(columnsPermissions[column] == true)
						{
							if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
							{
								MSG(boost::format("W: \"%s\": cell at row %d and column %d is undefined! Applying \"%s\".\n") % worksheetName % (row + 1) % (column + 1) % UNDEFINED_VALUE);
							}

							//XML:
							pugi::xml_attribute xmlAttribute = xmlItem.append_attribute(attributes[column].c_str());

							switch(cell->Type())
							{
							case ExcelFormat::BasicExcelCell::UNDEFINED:
								{
									//XML:
									xmlAttribute.set_value(UNDEFINED_VALUE);
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], UNDEFINED_VALUE));
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
									MSG(boost::format("E: spreadsheet \"%s\": cell at row %d and column %d is of unknown type. Applying \"%s\".\n") % worksheetName % (row + 1) % (column + 1) % UNKNOWN_VALUE);

									//XML:
									xmlAttribute.set_value(UNKNOWN_VALUE);
									//JSON:
									jsonItem.push_back(json_spirit::Pair(attributes[column], UNKNOWN_VALUE));
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
	FileOutput fileOutput("xls2xj.txt");
	messenger.add(&fileOutput);

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

	//итерация по всем .xls-файлам в текущей папке:
	int processed = 0;
	boost::filesystem::directory_iterator dirEnd;
	for(boost::filesystem::directory_iterator it("./"); it != dirEnd; it++)
	{
		if(boost::filesystem::is_directory(it->status()) == false)
		{
			std::string fileName = it->path().filename().string();
			const int size = fileName.size();
			//если файл формата .xls:
			if(size >= 4)
			{
				if(fileName[size - 4] == '.' && 
					(fileName[size - 3] == 'x' || fileName[size - 3] == 'X') &&
					(fileName[size - 2] == 'l' || fileName[size - 2] == 'L') &&
					(fileName[size - 1] == 's' || fileName[size - 1] == 'S'))
				{
					ProcessXLS(fileName);
					processed++;
				}
			}
		}
	}

	if(processed <= 0)
	{
		MSG(boost::format("W: no \"xls\" files found within executable's directory.\n"));
	}
	else
	{
		MSG(boost::format("I: All done. Hit Enter to exit...\n"));
	}

	getchar();

	return 0;
}

