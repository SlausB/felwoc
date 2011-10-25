

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>

#include <boost/lexical_cast.hpp>


std::string PrintType(Messenger& messenger, const int type)
{
	switch(type)
	{
	case Field::INHERITED:
		{

		}
		break;

	case Field::SERVICE:
		{
		}
		break;

	case Field::TEXT:
		{
		}
		break;

	case Field::FLOAT:
		{
		}
		break;

	case Field::INT:
		{
		}
		break;

	case Field::LINK:
		{
		}
		break;



	default:
		messenger.error(boost::format("E: AS3: PROGRAM ERROR: PrintType(): type = %d is undefined. Returning empty string. Refer to software supplier.\n") % type);
		return std::string();
	}
}

const char* everyonesParentName = "Info";


bool AS3Target::Generate(const AST& ast, Messenger& messenger)
{
	boost::filesystem::create_directory("./as3");
	boost::filesystem::create_directory("./as3/infos");

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

		std::string fileName = str(boost::format("./as3/infos/%s.as") % classNames[table]);
		std::ofstream file(fileName, std::ios_base::out);
		if(file.fail())
		{
			messenger << (boost::format("E: AS3: file \"%s\" was NOT opened.\n") % fileName);
			return false;
		}

		//file's head:
		{
			//intro:
			file << "/* This file is generated using the \"fxlsc\" program from XLS design file.\nBug issues or suggestions can be sent to SlavMFM@gmail.com\n*/\n\n";

			//doxygen:
			const char* doxygen = "// for doxygen to properly generate java-like documentation:\n";
			file << doxygen;
			file << "/// @cond\n";

			//package:
			file << "package infos {\n";

			//doxygen:
			file << indention << doxygen;
			file << indention << "/// @endcond\n";

			file << "\n\n";
		}

		//name:
		{
			std::string parentName = table->parent == NULL ? everyonesParentName : classNames[table->parent];
			file << indention << str(boost::format("class %s extends %s\n") % classNames[table] % parentName);
		}

		//body open:
		file << indention << "{\n";

		//fields:
		for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
		{
			Field* field = table->fields[fieldIndex];

			//commentary:
			file << indention << indention;
			file << str(boost::format("/** %s */\n") % field->commentary);

			//field:
			file << indention << indention;

			//switch(

			file << indention << indention << "\n";
		}

		//constructor:
		{
			const char* defaultConstructor = "Default constructor to let define all fields within any child classes derived from this class (if there are some).";
			file << indention << indention << defaultConstructor << "\n";
			file << indention << indention << classNames[table] << "()\n";
			file << indention << indention << "{\n";
			file << indention << indention << "}\n";
			file << indention << indention << "\n";

			//declaration:
			file << indention << indention << classNames[table] << "(";
			for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
			{
				if(fieldIndex > 0)
				{
					file << ", ";
				}

				Field* field = table->fields[fieldIndex];

				//file << field->name << ":" << 
			}
			file << indention << indention << ")";
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

