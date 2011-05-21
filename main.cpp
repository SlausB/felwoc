
#include "excel_format/ExcelFormat.h"
#include "excel_format/BasicExcel.hpp"
#include "xml/pugixml.hpp"
#include "json/json_spirit.h"

#include "output/messenger.h"
#include "output/outputs/console_output.h"

#include "Windows.h"

ConsoleOutput consoleOutput;
Messenger messenger(&consoleOutput);

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
	//������� ��� ���� � �����, ���� ����� �������� � ���� ���������� �����:
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

		//�� ������������� ������� �������, ������������ � ���������������� �����, ������ ������������ ��� �����������:
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

		//��������� ������ ������ ���������� �� ��������� (�������):
		std::map<int, bool> columnsPermissions;

		//�������� ���������:
		std::map<int, std::string> attributes;

		for(int row = 0; row < rowsCount; row++)
		{
			//������ ������ - ����������� ���������� �������:
			if(row < 1)
			{
				for(int column = 0; column < columnsCount; column++)
				{
					ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

					//������ ������� ��� �������������� ������������:
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
			//���� �� ������� ������ ��� ������� ���������:
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
				//���� ��� ������� ������ �� �������� ������������:
				if(worksheet->Cell(row, 0)->GetInteger() != 0)
				{
					//XML:
					pugi::xml_node xmlItem = xmlSpreadsheet.append_child("item");
					//JSON:
					json_spirit::Object jsonItem;

					//������ ������� �������������� ��� �������������� ������������:
					for(int column = 1; column < columnsCount; column++)
					{
						ExcelFormat::BasicExcelCell* cell = worksheet->Cell(row, column);

						//���� �������-������� ������� ������ �� �������� ������������:
						if(columnsPermissions[column] == true)
						{
							if(cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
							{
								MSG(boost::format("W: cell at row %d and column %d is undefined! Will be written as is.\n") % (row + 1) % (column + 1));
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
									//�������� ������� ������������ � ���� ������ ������ ���� �������� �� ��� ������:
									if(strlen(formulaAsString) <= 0)
									{
										std::string valueAsString = str(boost::format("%f") % cell->GetDouble());
										TruncateValue(valueAsString);
										
										//XML:
										xmlAttribute.set_value(valueAsString.c_str());
										//JSON:
										jsonItem.push_back(json_spirit::Pair(attributes[column], valueAsString));
									}
									//��������� ������� ����� �������������� ������:
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

	//������ � XML:
	xml.save_file(str(boost::format("%s.xml") % xlsName).c_str(), PUGIXML_TEXT("\t"), pugi::format_default, pugi::encoding_wchar);
	//������ � JSON:
	std::ofstream os(str(boost::format("%s.json") % xlsName).c_str());
	write(json, os, json_spirit::pretty_print | json_spirit::raw_utf8);

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
	//����� ������� �� �������� �� ���� ������ � ����� � ����������� ".xls"...
	ProcessXLS("design2.xls");

	MSG(boost::format("I: All done. Hit any key to exit...\n"));

	getchar();

	return 0;
}

