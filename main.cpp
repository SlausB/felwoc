
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
#include "targets/ts/ts_target.h"
#include "targets/haxe/haxe_target.h"

#include "parsing.h"

#include "data_source/ods_as_xml/ods_as_xml.h"

//print test:
#include <fstream>

ConsoleOutput consoleOutput;
Messenger messenger(&consoleOutput);

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
	FileOutput fileOutput("felwoc.txt");
	messenger.add(&fileOutput);

	//чтение конфигурационного файла:
	boost::property_tree::ptree config;
	std::string sourceFileName;
	const char* iniFileName = "felwoc.ini";
	try
	{
		boost::property_tree::read_ini(iniFileName, config);

		sourceFileName = config.get<std::string>("source");
	}
	catch(std::exception& e)
	{
		messenger << boost::format("E: configuration file \"%s\" was NOT loaded. Exception: \"%s\". Create proper configuration file and run again.\n") % iniFileName % e.what();
		getchar();
		return 1;
	}
	
	OdsAsXml xmlAsOds(sourceFileName.c_str(), &messenger);
	if(xmlAsOds.IsOk())
	{
		//add generators here:
		std::vector<TargetPlatform*> platforms;
		platforms.push_back( new AS3Target );
        platforms.push_back( new TS_Target );
        platforms.push_back( new Haxe_Target );

		for(int i = 0; i < xmlAsOds.GetSpreadsheetsCount(); i++)
		{
			boost::shared_ptr<Spreadsheet> spreadsheet = xmlAsOds.GetSpreadsheet(i);
			if(spreadsheet->GetName().compare("Constants") == 0)
			{
				boost::shared_ptr<Cell> cell = spreadsheet->GetCell(0, 1);
				break;
			}
		}
		
		AST ast;
		Parsing parsing;
		if(parsing.ProcessSource(ast, messenger, &xmlAsOds))
		{
			messenger.info(boost::format("Successfully compiled.\n"));

			for(size_t generatorIndex = 0; generatorIndex < platforms.size(); generatorIndex++)
			{
				platforms[generatorIndex]->Generate(ast, messenger, config);
			}
		}
	}

    //why?
	//getchar();

	return 0;
}

