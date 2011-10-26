

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>

#include <boost/lexical_cast.hpp>


const char* everyonesParentName = "Info";
	
const std::string indention = "	";

const char* linkName = "Link";
const char* countName = "Count";

const char* findLinkTargetFunctionName = "FindLinkTarget";

const char* allTablesName = "__all";


/** Returns empty string on some error.*/
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

std::string PrintData(Messenger& messenger, const FieldData* fieldData)
{
	switch(fieldData->field->type)
	{
	case Field::INHERITED:
		{
			Inherited* inherited = (Inherited*)fieldData;
			return PrintData(messenger, inherited->fieldData);
		}
		break;

	case Field::SERVICE:
		{
			Service* service = (Service*)fieldData;
			ServiceField* serviceField = (ServiceField*)fieldData->field;
			switch(serviceField->serviceType)
			{
			case ServiceField::ID:
				{
					Int* asInt = (Int*)service->fieldData;
					return boost::lexical_cast<std::string>(asInt->value);
				}

			default:
				messenger.error(boost::format("E: AS3: PROGRAM ERROR: PrintData(): service type %d is undefined. Refer to software supplier.\n") % serviceField->serviceType);
			}
		}
		break;

	case Field::TEXT:
		{
			Text* text = (Text*)fieldData;
			return text->text;
		}
		break;

	case Field::FLOAT:
		{
			Float* asFloat = (Float*)fieldData;
			return boost::lexical_cast<std::string>(asFloat->value);
		}
		break;

	case Field::INT:
		{
			Int* asInt = (Int*)fieldData;
			return boost::lexical_cast<std::string>(asInt->value);
		}
		break;

	case Field::LINK:
		{
			//links connected later:
			return "NULL";
		}
		break;

	case Field::BOOL:
		{
			Bool* asBool = (Bool*)fieldData;
			if(asBool->value)
			{
				return "true";
			}
			return "false";
		}
		break;

	case Field::ARRAY:
		{
			Array* asArray = (Array*)fieldData;

			std::string arrayData = "[";
			for(size_t i = 0; i < asArray->values.size(); i++)
			{
				if(i > 0)
				{
					arrayData.append(", ");
				}

				arrayData.append(boost::lexical_cast<std::string>(asArray->values[i]));
			}
			arrayData.append("]");

			return arrayData;
		}
		break;

	
	default:
		messenger.error(boost::format("E: AS3: PROGRAM ERROR: PrintData(): type = %d is undefined. Returning empty string. Refer to software supplier.\n") % fieldData->field->type);
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
	const std::string targetFolder = "./as3";
	const std::string explanation = "/* This file is generated using the \"fxlsc\" program from XLS design file.\nBug issues or suggestions can be sent to SlavMFM@gmail.com\n*/\n\n";
	const char* doxygen = "// for doxygen to properly generate java-like documentation:\n";
	const char* doxygen_cond = "/// @cond\n";
	const char* doxygen_endcond = "/// @endcond\n";
	const char* packageName = "infos";

	boost::filesystem::create_directory(targetFolder);
	boost::filesystem::create_directory(targetFolder + "/infos");

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

		std::string fileName = str(boost::format("%s/infos/%s.as") % targetFolder % classNames[table]);
		std::ofstream file(fileName, std::ios_base::out);
		if(file.fail())
		{
			messenger << (boost::format("E: AS3: file \"%s\" was NOT opened.\n") % fileName);
			return false;
		}

		//file's head:
		{
			//intro:
			file << explanation;

			//doxygen:
			file << doxygen;
			file << doxygen_cond;

			//package:
			file << "package " << packageName << "\n{\n";

			//doxygen:
			file << indention << doxygen;
			file << indention << doxygen_endcond;

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
			file << indention << indention << "public ";
			if(table->type == Table::PRECISE)
			{
				file << "static const ";
			}
			else
			{
				file << "var ";
			}
			file << field->name << ":";
			if(field->type == Field::LINK)
			{
				file << linkName;
			}
			else
			{
				file << fieldsTypeName;
			}

			if(table->type == Table::PRECISE)
			{
				if(field->type != Field::LINK)
				{
					file << " = " << PrintData(messenger, table->matrix[0][fieldIndex]);
				}
			}

			file << ";\n";


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
			bool once = false;
			for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
			{
				Field* field = table->fields[fieldIndex];

				//links are defined after all initializations:
				if(field->type == Field::LINK)
				{
					continue;
				}

				if(once)
				{
					file << ", ";
				}
				once = true;

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

	//incapsulation:
	{
		const char* incapsulationName = "Infos";

		std::string fileName = str(boost::format("%s/%s.as") % targetFolder % incapsulationName);
		std::ofstream file(fileName.c_str());
		if(file.fail())
		{
			messenger << (boost::format("E: AS3: file \"%s\" was NOT opened.\n") % fileName);
			return false;
		}
		
		//file's head:
		{
			//intro:
			file << explanation;

			//doxygen:
			file << doxygen;
			file << doxygen_cond;

			//package:
			file << "package\n{\n";

			//imports:
			bool once = false;
			for(std::map<Table*, std::string>::const_iterator it = classNames.begin(); it != classNames.end(); it++)
			{
				file << indention << "import " << packageName << "." << it->second << ";\n";
				once = true;
			}
			if(once)
			{
				file << indention << "\n";
			}

			//doxygen:
			file << indention << doxygen;
			file << indention << doxygen_endcond;

			file << indention << "\n";
			file << indention << "\n";
		}

		//name:
		{
			file << indention << "/** Data definition.*/\n";
			file << indention << "class " << incapsulationName;
		}

		//body open:
		file << indention << "{\n";

		//fields:
		for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
		{
			Table* table = ast.tables[tableIndex];

			if(table->type == Table::VIRTUAL)
			{
				continue;
			}

			//commentary:
			file << PrintCommentary(indention + indention, table->commentary);

			//as field:
			file << indention << indention << "public var " << table->lowercaseName;
			switch(table->type)
			{
			case Table::MANY:
			//for now there are no differences with MANY:
			case Table::MORPH:
				{
					file << ":Array = [];\n";
				}
				break;

			case Table::PRECISE:
			//for now there are no differences with PRECISE:
			case Table::SINGLE:
				{
					file << str(boost::format(":%s;\n") % classNames[table]);
				}
				break;

			default:
				messenger.error(boost::format("E: AS3: table of type %d with name \"%s\" was NOT implemented. Refer to software supplier.\n") % table->type % table->realName);
				return false;
			}

			file << indention << indention << "\n";
		}
		//all:
		file << indention << indention << "public var " << allTablesName << ":Vector.<Vector.<" << everyonesParentName << "> > = [];\n\n";

		//constructor:
		//initialization:
		file << indention << indention << "/** All data definition.*/\n";
		file << indention << indention << incapsulationName << "()\n";
		//definition:
		file << indention << indention << "{\n";
		for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
		{
			Table* table = ast.tables[tableIndex];
			if(table->type != Table::MANY && table->type != Table::MORPH)
			{
				continue;
			}

			if(tableIndex > 0)
			{
				file << indention << indention << indention << "\n";
			}

			//data itself:
			for(std::vector<std::vector<FieldData*> >::const_iterator row = table->matrix.begin(); row != table->matrix.end(); row++)
			{
				file << indention << indention << indention << table->lowercaseName << ".push(new " << classNames[table] << "(";

				bool once = false;
				for(std::vector<FieldData*>::const_iterator column = row->begin(); column != row->end(); column++)
				{
					FieldData* fieldData = *column;

					if(fieldData->field->type == Field::LINK)
					{
						continue;
					}

					if(once)
					{
						file << ", ";
					}
					once = true;

					file << PrintData(messenger, fieldData);
				}

				file << "));\n";
			}
		}
		file << indention << indention << indention << "\n";
		//connect links:
		{
			file << indention << indention << indention << "//connect links:\n";

			for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
			{
				Table* table = ast.tables[tableIndex];

				if(table->type != Table::MANY && table->type != Table::MORPH)
				{
					continue;
				}

				bool atLeastOne = false;

				for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
				{
					Field* field = table->fields[fieldIndex];

					if(field->type != Field::LINK)
					{
						continue;
					}

					atLeastOne = true;
				
					int rowIndex = 0;
					for(std::vector<std::vector<FieldData*> >::const_iterator row = table->matrix.begin(); row != table->matrix.end(); row++, rowIndex++)
					{
						Link* link = (Link*)(row->at(fieldIndex));

						file << indention << indention << indention << table->lowercaseName << "[" << rowIndex << "] = new " << linkName << "([";
						for(std::vector<Count>::const_iterator count = link->links.begin(); count != link->links.end(); count++)
						{
							if(count != link->links.begin())
							{
								file << ", ";
							}

							file << "new " << countName << "(" << everyonesParentName << "." << findLinkTargetFunctionName << "(" << classNames[count->table] << ", " << count->id << "), " << count->count << ")";
						}
						file << "]);\n";
					}
				}

				if(atLeastOne)
				{
					file << indention << indention << indention << "\n";
				}
			}
		}
		file << indention << indention << indention << "\n";
		//all manys:
		{
			file << indention << indention << indention << "//all tables of type \"many\":\n";
			for(size_t tableIndex = 0; tableIndex < ast.tables.size(); tableIndex++)
			{
				Table* table = ast.tables[tableIndex];

				if(table->type != Table::MANY)
				{
					continue;
				}

				file << indention << indention << indention << allTablesName << ".push(" << table->lowercaseName << ");\n";
			}
		}
		file << indention << indention << "}\n";

		//function which finds links targets:
		{
			file << indention << indention << "/** Looks for link target.*/\n";
			file << indention << indention << "public static function " << findLinkTargetFunctionName << "(where:Vector.<" << everyonesParentName << ">, id:int)\n";
			file << indention << indention << "{\n";
			file << indention << indention << indention << "for(var i:int = 0; i < where.length; i++)\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "if(where[i].id == id) return where[i];\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "return NULL;\n";
			file << indention << indention << "}\n";
		}


		//body close:
		file << indention << "}\n";

		//package close:
		file << "}\n\n";
	}

	//link, count and everyones parent types declaration:
	{
		std::string linkFileName = str(boost::format("%s/%s.as") % targetFolder % linkName);
		std::ofstream linkFile(linkFileName.c_str());
		if(linkFile.fail())
		{
			messenger << (boost::format("E: AS3: file \"%s\" was NOT opened.\n") % linkFileName);
			return false;
		}
		
		std::string countFileName = str(boost::format("%s/%s.as") % targetFolder % countName);
		std::ofstream countFile(countFileName.c_str());
		if(countFile.fail())
		{
			messenger << (boost::format("E: AS3: file \"%s\" was NOT opened.\n") % countFileName);
			return false;
		}

		std::string everyonesParentFileName = str(boost::format("%s/%s.as") % targetFolder % everyonesParentName);
		std::ofstream everyonesParentFile(everyonesParentFileName.c_str());
		if(everyonesParentFile.fail())
		{
			messenger << (boost::format("E: AS3: file \"%s\" was NOT opened.\n") % everyonesParentFile);
			return false;
		}
		
		//file's head:
		{
			//intro:
			linkFile << explanation;
			countFile << explanation;
			everyonesParentFile << explanation;

			//doxygen:
			linkFile << doxygen;
			countFile << doxygen;
			everyonesParentFile << doxygen;
			linkFile << doxygen_cond;
			countFile << doxygen_cond;
			everyonesParentFile << doxygen_cond;

			//package:
			linkFile << "package\n{\n";
			countFile << "package\n{\n";
			everyonesParentFile << "package\n{\n";

			//doxygen:
			linkFile << indention << doxygen;
			countFile << indention << doxygen;
			everyonesParentFile << indention << doxygen;
			linkFile << indention << doxygen_endcond;
			countFile << indention << doxygen_endcond;
			everyonesParentFile << indention << doxygen_endcond;

			linkFile << "\n\n";
			countFile << "\n\n";
			everyonesParentFile << "\n\n";
		}

		//name:
		{
			linkFile << indention << "/** Field that incapsulates all objects and counts to which object was linked*/\n";
			countFile << indention << "/** Link to single object which holds object's count along with object's pointer.*/\n";
			everyonesParentFile << indention << "/** All design types inherited from this class to permit objects storage within single array.*/\n";
			linkFile << indention << "class " << linkName;
			countFile << indention << "class " << countName;
			everyonesParentFile << indention << "class " << everyonesParentName;
		}

		//body open:
		linkFile << indention << "{\n";
		countFile << indention << "{\n";
		everyonesParentFile << indention << "{\n";

		//fields:
		//link-specific:
		linkFile << indention << indention << str(boost::format("/** All linked objects. Elements of type \"%s\". Defined within constructor.\n") % countName);
		linkFile << indention << indention << "public var links:Vector.<" << everyonesParentName << ">;\n";
		//count-specific:
		countFile << indention << indention << str(boost::format("/** Linked object. Defined within constructor.\n"));
		countFile << indention << indention << str(boost::format("public var object:%s;\n") % everyonesParentName);
		countFile << indention << indention << str(boost::format("/** Linked objects count. If count was not specified within XLS then 1. Interpretation depends on game logic.\n"));
		countFile << indention << indention << str(boost::format("public var count:int;\n"));
		//everyones-parent specific:
		//...


		//constructor:

		//initialization:
		linkFile << indention << indention << "/** All counts at once.*/\n";
		countFile << indention << indention << "/** All pointer and count at construction.*/\n";
		linkFile << indention << indention << linkName << "(links:Vector.<" << everyonesParentName << ">)\n";
		countFile << indention << indention << countName << str(boost::format("(object:%s, count:int)\n") % everyonesParentName);

		//definition:
		linkFile << indention << indention << "{\n";
		countFile << indention << indention << "{\n";

		//definition:
		//link-specific:
		linkFile << indention << indention << indention << "this.links = links;\n";
		//count-specific:
		countFile << indention << indention << indention << "this.object = object;\n";
		countFile << indention << indention << indention << "this.count = count;\n";

		linkFile << indention << indention << "}\n";
		countFile << indention << indention << "}\n";


		//body close:
		linkFile << indention << "}\n";
		countFile << indention << "}\n";
		everyonesParentFile << indention << "}\n";

		//package close:
		linkFile << "}\n\n";
		countFile << indention << "}\n\n";
		everyonesParentFile << indention << "}\n\n";
	}


	messenger << (boost::format("I: AS3 code successfully generated.\n"));

	return true;
}

