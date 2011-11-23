
#include "excel_format/ExcelFormat.h"
#include "excel_format/BasicExcel.hpp"

/*#include "xml/pugixml.hpp"
#include "json/json_spirit.h"*/

#include "output/messenger.h"
#include "output/outputs/console_output.h"
#include "output/outputs/file_output.h"

//configuration file:
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <boost/filesystem.hpp>

#include "targets/target_platform.h"
#include "targets/as3/as3_target.h"

#include "parsing.h"

#include "data_source/ods_as_xml/ods_as_xml.h"

ConsoleOutput consoleOutput;
Messenger messenger(&consoleOutput);


char* UNDEFINED_NAME = "__undefined_name__";
char* UNDEFINED_VALUE = "__undefined_value__";
char* NULL_NAME = "__null_name__";
char* UNKNOWN_VALUE = "__unknown_value__";

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

int main()
{
	FileOutput fileOutput("fxlsc.txt");
	messenger.add(&fileOutput);

	//чтение конфигурационного файла:
	boost::property_tree::ptree config;
	std::string sourceFileName;
	const char* iniFileName = "fxlsc.ini";
	try
	{
		boost::property_tree::read_ini(iniFileName, config);

		sourceFileName = config.get<std::string>("source");
	}
	catch(std::exception& e)
	{
		messenger << boost::format("E: \"%s\" was NOT loaded. Exception: \"%s\". Proceeding with the default settings.\n") % iniFileName % e.what();
	}
	
	//add generators here:
	std::vector<TargetPlatform*> platforms;
	platforms.push_back(new AS3Target);
	
	
	Parsing parsing;
	
	//теперь только один файл - указанный в инициализационном файле:
	/*//итерация по всем .xls-файлам в текущей папке:
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
					AST ast;
					ast.fileName = fileName;
					messenger.info(boost::format("------------------------------\nFile \"%s\":\n") % fileName);
					if(parsing.ProcessXLS(ast, messenger, fileName))
					{
						messenger.info(boost::format("Successfully compiled.\n"));

						for(size_t generatorIndex = 0; generatorIndex < platforms.size(); generatorIndex++)
						{
							platforms[generatorIndex]->Generate(ast, messenger, config);
						}
					}
					processed++;
				}
			}
		}
	}*/
	OdsAsXml xmlAsOds(sourceFileName.c_str());

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

