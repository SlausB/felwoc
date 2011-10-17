

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>


bool AS3Target::Generate(const AST& ast, Messenger& messenger)
{
	boost::filesystem::create_directory("as3");

	//make class names:
	std::map<Table*, std::string> classNames;
	for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
	{
		Table* table = ast.tables[tableIndex];
		std::string className = table->lowercaseName;
		std::transform(className.begin(), ++(className.begin()), className.begin(), ::toupper);
		classNames[table] = className;
	}

	//generate classes:
	for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
	{
		Table* table = ast.tables[tableIndex];

		std::string fileName = str(boost::format("as3/%s.as") % classNames[table]);
		std::ofstream file(fileName, std::ios_base::out);
		if(file.fail())
		{
			messenger << (boost::format("E: %s: AS3: file \"%s\" was NOT opened.\n") % ast.fileName % fileName);
			return false;
		}

		//name:
		if(table->parent == NULL)
		{
			file << str(boost::format("class %s\n") % classNames[table]);
		}
		else
		{
			file << str(boost::format("class %s extends %s\n") % classNames[table] % classNames[table->parent]);
		}

		//body:
		file << "{\n}\n\n";
	}


	//generate data:
	for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
	{
	}

	messenger << (boost::format("I: AS3 code successfully generated.\n"));

	return true;
}

