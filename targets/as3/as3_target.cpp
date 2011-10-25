

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>

#include <boost/lexical_cast.hpp>


const char* everyonesParentName = "Info";


std::string PrintType(Messenger& messenger, const Field* field)
{
	switch(field->type)
	{
	case Field::INHERITED:
		{
			InheritedField* inheritedField = (InheritedField*)field;
			return PrintType(messenger, inheritedField->parentField);
		}
		break;

	case Field::SERVICE:
		{
			ServiceField* serviceField = (ServiceField*)field;
			switch(serviceField->serviceType)
			{
			case ServiceField::ID:
				return "int";

			default:
				messenger.error(boost::format("E: AS3: PROGRAM ERROR: PrintType(): service type = %d is undefined. Returning empty string. Refer to software supplier.\n") % serviceField->serviceType);
				return std::string();
			}
		}
		break;

	case Field::TEXT:
		{
			return "String";
		}
		break;

	case Field::FLOAT:
		{
			return "Number";
		}
		break;

	case Field::INT:
		{
			return "int";
		}
		break;

	case Field::LINK:
		{
			return everyonesParentName;
		}
		break;

	case Field::BOOL:
		{
			return "Boolean";
		}
		break;

	case Field::ARRAY:
		{
			return "Array";
		}
		break;

	
	default:
		messenger.error(boost::format("E: AS3: PROGRAM ERROR: PrintType(): type = %d is undefined. Returning empty string. Refer to software supplier.\n") % field->type);
	}

	return std::string();
}

/** Prints simple or multiline commentary with specified indention.*/
std::string PrintCommentary(const std::string& indention, const std::string& commentary)
{
	std::string result;
	
	result.append(indention);
	result.append("/** ");
				
	bool multiline = false;
	for(size_t i = 0; i < commentary.size(); i++)
	{
		if(commentary[i] == '\n')
		{
			multiline = true;
			break;
		}
	}

	if(multiline)
	{
		result.append("\n");
		result.append(indention);

		bool lastIsEscape = false;
		for(size_t i = 0; i < commentary.size(); i++)
		{
			result.push_back(commentary[i]);

			if(commentary[i] == '\n')
			{
				result.append(indention);

				lastIsEscape = true;
			}
			else
			{
				lastIsEscape = false;
			}
		}
		if(lastIsEscape == false)
		{
			result.append("\n");
		}

		result.append(indention);
	}
	else
	{
		result.append(commentary);
		result.append(" ");
	}

	result.append("*/\n");

	return result;
}


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
			file << "package infos\n{\n";

			//doxygen:
			file << indention << doxygen;
			file << indention << "/// @endcond\n";

			file << "\n\n";
		}

		//name:
		{
			file << PrintCommentary(indention, table->commentary);

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
			file << PrintCommentary(indention + indention, field->commentary);

			//field:
			const std::string fieldsTypeName = PrintType(messenger, field);
			if(fieldsTypeName.empty())
			{
				return false;
			}
			file << indention << indention << "public var " << field->name << ":" << fieldsTypeName << ";\n";

			file << indention << indention << "\n";
		}

		//constructor:
		{
			//default:
			file << indention << indention << "/** Default constructor to let define all fields within any child classes derived from this class (if there are some). */\n";
			file << indention << indention << classNames[table] << "()\n";
			file << indention << indention << "{\n";
			file << indention << indention << "}\n";
			file << indention << indention << "\n";

			//initialization:
			//declaration:
			file << indention << indention << classNames[table] << "(";
			for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
			{
				if(fieldIndex > 0)
				{
					file << ", ";
				}

				Field* field = table->fields[fieldIndex];

				const std::string fieldsTypeName = PrintType(messenger, field);
				if(fieldsTypeName.empty())
				{
					return false;
				}
				file << field->name << ":" << fieldsTypeName;
			}
			file << ")\n";
		}
		//definition:
		file << indention << indention << "{\n";
		for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
		{
			Field* field = table->fields[fieldIndex];

			file << indention << indention << indention << "this." << field->name << " = " << field->name << ";\n";
		}
		file << indention << indention << "}\n";



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

