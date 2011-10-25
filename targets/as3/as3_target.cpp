

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>

#include <boost/lexical_cast.hpp>


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
	
	std::string indention = "	";

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

		//package open:
		file << "package {\n";

		//name:
		if(table->parent == NULL)
		{
			file << str(boost::format("%sclass %s\n") % indention % classNames[table]);
		}
		else
		{
			file << str(boost::format("%sclass %s extends %s\n") % indention % classNames[table] % classNames[table->parent]);
		}

		//body open:
		file << indention << "{\n";

		//fields:
		for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
		{
			if(fieldIndex > 0)
			{
				file << indention << indention << "\n";
			}

			Field* field = table->fields[fieldIndex];

			//commentary:
			file << indention << indention;
			file << str(boost::format("/** %s */\n") % field->commentary);

			//field:
			file << indention << indention;

			//switch(
		}



		//body close:
		file << indention << "}\n";

		//package close:
		file << "}\n\n";
	}

/*	//incapsulation:
	std::string incFileName = str(boost::format("as3/%s.as") % Unique("Infos", 0, classNames));
	std::ofstream incFile(incFileName.c_str());
	if(incFile.fail())
	{
		messenger << (boost::format("E: %s: AS3: file \"%s\" was NOT opened.\n") % ast.fileName % incFileName);
		return false;
	}
	*/


	messenger << (boost::format("I: AS3 code successfully generated.\n"));

	return true;
}

