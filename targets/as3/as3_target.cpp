﻿

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>

#include <boost/lexical_cast.hpp>

#include "../../smhasher/MurmurHash3.h"


std::string targetFolder;

const char* everyonesParentName = "Info";
	
const std::string indention = "	";

const char* linkName = "Link";
const char* countName = "Count";

const char* findLinkTargetFunctionName = "FindLinkTarget";

const char* allTablesName = "__all";

/** Package where all code will be put.*/
std::string overallNamespace;

const std::string explanation = "/* This file is generated using the \"fxlsc\" program from XLS design file.\nBugs issues or suggestions can be sent to SlavMFM@gmail.com\n*/\n\n";

const char* doxygen = "// for doxygen to properly generate java-like documentation:\n";
const char* doxygen_cond = "/// @cond\n";
const char* doxygen_endcond = "/// @endcond\n";

const char* packageName = "infos";

//appended to all class names:
const char* postfix = "Info";

//name of class which incapsulates table properties and array of it's objects:
const char* boundName = "Bound";

const std::string tabName = "__tabName";




bool IsLink(Field* field)
{
	if(field->type == Field::LINK)
	{
		return true;
	}

	if(field->type == Field::INHERITED)
	{
		if(((InheritedField*)field)->parentField->type == Field::LINK)
		{
			return true;
		}
	}

	return false;
}

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

			std::string result = "\"";
			//escape:
			for(size_t i = 0; i < text->text.size(); i++)
			{
				if(text->text[i] == '"')
				{
					result.push_back('\\');
				}

				result.push_back(text->text[i]);
			}
			result.push_back('"');
			return result;
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
			//links will be connected later:
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
			for ( size_t i = 0; i < asArray->values.size(); ++i )
			{
				if ( i > 0 )
				{
					arrayData.append( ", " );
				}

				arrayData.append( boost::lexical_cast< std::string >( asArray->values[ i ] ) );
			}
			arrayData.append( "]" );

			return arrayData;
		}
		break;

	
	default:
		messenger.error( boost::format( "E: AS3: PROGRAM ERROR: PrintData(): type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % fieldData->field->type );
	}

	return std::string();
}

/** Prints simple or multiline commentary with specified indention.*/
std::string PrintCommentary( const std::string& indention, const std::string& commentary )
{
	std::string result;
	
	result.append( indention );
	result.append( "/** " );
				
	bool multiline = false;
	for ( size_t i = 0; i < commentary.size(); ++i )
	{
		if ( commentary[ i ] == '\n' )
		{
			multiline = true;
			break;
		}
	}

	if ( multiline )
	{
		result.append( "\n" );
		result.append( indention );

		bool lastIsEscape = false;
		for ( size_t i = 0; i < commentary.size(); ++i )
		{
			result.push_back( commentary[ i ] );

			if ( commentary[ i ] == '\n' )
			{
				result.append( indention );

				lastIsEscape = true;
			}
			else
			{
				lastIsEscape = false;
			}
		}
		if ( lastIsEscape == false )
		{
			result.append( "\n" );
		}

		result.append( indention );
	}
	else
	{
		result.append( commentary );
		result.append( " " );
	}

	result.append( "*/\n" );

	return result;
}


bool AS3Target::Generate( const AST& ast, Messenger& messenger, const boost::property_tree::ptree& config )
{
	overallNamespace = config.get< std::string >( "namespace", "design" );
	targetFolder = config.get< std::string >( "as3_target_folder", "./as3/code/design" );
	//get rid of last folder separator:
	if ( targetFolder.empty() == false )
	{
		char last = targetFolder[ targetFolder.size() - 1 ];
		if ( last == '/' || last == '\\' )
		{
			targetFolder.resize( targetFolder.size() - 1 );
		}
	}

	boost::filesystem::create_directories( targetFolder );
	boost::filesystem::create_directory( targetFolder + "/infos" );

	//make class names:
	std::map< Table*, std::string > classNames;
	for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
	{
		Table* table = ast.tables[tableIndex];
		std::string className = table->lowercaseName;
		std::transform(className.begin(), ++(className.begin()), className.begin(), ::toupper);
		className.append(postfix);
		classNames[table] = className;
	}

	//generate classes:
	for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
	{
		Table* table = ast.tables[ tableIndex ];

		std::string fileName = str( boost::format( "%s/infos/%s.as" ) % targetFolder % classNames[ table ] );
		std::ofstream file( fileName, std::ios_base::out );
		if ( file.fail() )
		{
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % fileName );
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
			file << "package " << overallNamespace << "." << packageName << "\n{\n";

			//imports:
			file << indention << "import " << overallNamespace << "." << linkName << ";\n";
			file << indention << "import " << overallNamespace << "." <<  everyonesParentName << ";\n";
			file << indention << "\n";

			//doxygen:
			file << indention << doxygen;
			file << indention << doxygen_endcond;

			file << indention << "\n";
			file << indention << "\n";
		}

		//name:
		{
			file << PrintCommentary( indention, table->commentary );

			std::string parentName = table->parent == NULL ? everyonesParentName : classNames[ table->parent ];
			file << indention << str( boost::format( "public class %s extends %s\n" ) % classNames[ table ] % parentName );
		}

		//body open:
		file << indention << "{\n";

		//fields:
		for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex )
		{
			Field* field = table->fields[ fieldIndex ];

			if ( field->type == Field::INHERITED )
			{
				continue;
			}

			//commentary:
			file << PrintCommentary( indention + indention, field->commentary );

			//field:

			const std::string fieldsTypeName = PrintType( messenger, field );
			if ( fieldsTypeName.empty() )
			{
				return false;
			}
			file << indention << indention << "public ";
			if ( table->type == Table::PRECISE )
			{
				file << "static const ";
			}
			else
			{
				file << "var ";
			}
			file << field->name << ":";
			if ( IsLink( field ) )
			{
				file << linkName;
			}
			else
			{
				file << fieldsTypeName;
			}

			if ( table->type == Table::PRECISE )
			{
				if ( IsLink( field ) == false )
				{
					file << " = " << PrintData( messenger, table->matrix[ 0 ][ fieldIndex ] );
				}
			}

			file << ";\n";


			file << indention << indention << "\n";
		}

		//constructor:
		if ( table->type != Table::PRECISE && table->type != Table::SINGLE && table->type != Table::VIRTUAL )
		{
			//declaration:
			file << indention << indention << "/** All (including inherited) fields (excluding links) are defined here. To let define classes only within it's classes without inherited constructors used udnefined arguments.*/\n";
			file << indention << indention << "public function " << classNames[table] << "(";
			/*bool once = false;
			for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
			{
				Field* field = table->fields[fieldIndex];

				//links are defined after all initializations:
				if(IsLink(field))
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
			}*/
			file << " ... args ";
			file << ")\n";
		

			//definition:
			file << indention << indention << "{\n";
			file << indention << indention << indention << "if ( args.length <= 0 )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "return;\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "\n";
			file << indention << indention << indention << "this." << tabName << " = args[ 0 ];\n";
			int argIndex = 1;
			for(size_t fieldIndex = 0; fieldIndex < table->fields.size(); fieldIndex++)
			{
				Field* field = table->fields[fieldIndex];

				if(IsLink(field))
				{
					continue;
				}
	
				//file << indention << indention << indention << "this." << field->name << " = " << field->name << ";\n";
				file << indention << indention << indention << "this." << field->name << " = args[" << argIndex << "];\n";
				argIndex++;
			}
			file << indention << indention << "}\n";
		}
		

		//body close:
		file << indention << "}\n";

		//package close:
		file << "}\n\n";
	}

	//incapsulation:
	{
		const char* incapsulationName = "Infos";

		std::string fileName = str( boost::format( "%s/%s.as" ) % targetFolder % incapsulationName );
		std::ofstream file( fileName.c_str() );
		if ( file.fail() )
		{
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % fileName );
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
			file << "package " << overallNamespace << "\n";
			file << "{\n";

			//imports:
			{
				//obligatory imports:
				file << indention << "import " << overallNamespace << "." << linkName << ";\n";
				file << indention << "import " << overallNamespace << "." << everyonesParentName << ";\n";
				file << indention << "import " << overallNamespace << "." << boundName << ";\n";
				file << indention << "\n";

				//infos:
				bool once = false;
				for(std::map<Table*, std::string>::const_iterator it = classNames.begin(); it != classNames.end(); it++)
				{
					file << indention << "import " << overallNamespace << "." << packageName << "." << it->second << ";\n";
					once = true;
				}
				if(once)
				{
					file << indention << "\n";
				}
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
			file << indention << "public class " << incapsulationName << "\n";
		}

		//body open:
		file << indention << "{\n";

		//fields:
		for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
		{
			Table* table = ast.tables[ tableIndex ];

			if ( table->type == Table::VIRTUAL )
			{
				continue;
			}

			//commentary:
			file << PrintCommentary( indention + indention, table->commentary );

			//as field:
			file << indention << indention << "public var " << table->lowercaseName;
			switch ( table->type )
			{
			case Table::MANY:
			//for now there are no differences with MANY:
			case Table::MORPH:
				{
					file << ":Vector.< " << classNames[ table ] << " > = new Vector.< " << classNames[ table ] << " >;\n";
				}
				break;

			case Table::PRECISE:
			//for now there are no differences with PRECISE:
			case Table::SINGLE:
				{
					file << str( boost::format( ":%s;\n" ) % classNames[ table ] );
				}
				break;

			default:
				messenger.error( boost::format( "E: AS3: table of type %d with name \"%s\" was NOT implemented. Refer to software supplier.\n" ) % table->type % table->realName );
				return false;
			}

			file << indention << indention << "\n";
		}
		//all:
		file << indention << indention << "/** Array of all arrays. Each element is of type Vector.< " << everyonesParentName << " > */\n";
		file << indention << indention << "public var " << allTablesName << ":Vector.< " << boundName << " > = new Vector.< " << boundName << " >;\n";
		file << indention << indention << "\n";

		//constructor:
		//initialization:
		file << indention << indention << "/** All data definition.*/\n";
		file << indention << indention << "public function " << incapsulationName << "()\n";
		//definition:
		file << indention << indention << "{\n";
		bool somethingOut = false;
		for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
		{
			Table* table = ast.tables[ tableIndex ];
			if ( table->type != Table::MANY && table->type != Table::MORPH )
			{
				continue;
			}

			if ( somethingOut )
			{
				file << indention << indention << indention << "\n";
				somethingOut = false;
			}

			//data itself:
			for ( std::vector<std::vector< FieldData* > >::const_iterator row = table->matrix.begin(); row != table->matrix.end(); ++row )
			{
				file << indention << indention << indention << table->lowercaseName << ".push( new " << classNames[ table ] << "( ";

				file << "\"" << table->realName << "\"";

				for( std::vector< FieldData* >::const_iterator column = row->begin(); column != row->end(); ++column )
				{
					FieldData* fieldData = *column;

					if ( IsLink( fieldData->field ) )
					{
						continue;
					}

					file << ", " << PrintData( messenger, fieldData );
				}

				file << " ) );\n";

				somethingOut = true;
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

					if(IsLink(field) == false)
					{
						continue;
					}

					atLeastOne = true;
				
					int rowIndex = 0;
					for ( std::vector< std::vector< FieldData* > >::const_iterator row = table->matrix.begin(); row != table->matrix.end(); ++row, ++rowIndex )
					{
						Link* link;
						if ( field->type == Field::LINK )
						{
							link = ( Link* ) ( row->at( fieldIndex ) );
						}
						else if ( field->type == Field::INHERITED )
						{
							link = ( Link* ) ( ( ( Inherited* ) ( row->at( fieldIndex ) ) )->fieldData );
						}
						else
						{
							messenger.error( boost::format( "E: AS3: PROGRAM ERROR: linking: field's type = %d is undefined. Refer to software supplier.\n" ) % field->type );
							return false;
						}

						file << indention << indention << indention << table->lowercaseName << "[ " << rowIndex << " ]." << field->name << " = new " << linkName << "( [ ";
						for ( std::vector< Count >::const_iterator count = link->links.begin(); count != link->links.end(); ++count )
						{
							if ( count != link->links.begin() )
							{
								file << ", ";
							}

							file << "new " << countName << "( " << incapsulationName << "." << findLinkTargetFunctionName << "( " << count->table->lowercaseName << ", " << count->id << " ), " << count->count << " )";
						}
						file << " ] );\n";
					}
				}

				if ( atLeastOne )
				{
					file << indention << indention << indention << "\n";
				}
			}
		}
		file << indention << indention << indention << "\n";
		//all manys:
		{
			std::map< uint32_t, std::string > usedHashes;

			file << indention << indention << indention << "//all tables of type \"many\":\n";
			for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
			{
				Table* table = ast.tables[ tableIndex ];
				
				uint32_t hash;
				MurmurHash3_x86_32( table->lowercaseName.c_str(), table->lowercaseName.size(), 0, &hash );
				//check that generated hash value doesn't collides with all already generated hash values:
				for ( std::map< uint32_t, std::string >::iterator it = usedHashes.begin(); it != usedHashes.end(); ++it )
				{
					if ( hash == it->first )
					{
						messenger << ( boost::format( "E: AS3: hash value generated from table's name \"%s\" collides with one generated from \"%s\". It happens once per ~100000 cases, so you're very \"lucky\" to face it. Change one of these table names' to something else to avoid this problem.\n" ) % table->lowercaseName % it->second );
						return false;
					}
				}
				usedHashes[ hash ] = table->lowercaseName;

				if ( table->type != Table::MANY )
				{
					continue;
				}

				file << indention << indention << indention << allTablesName << ".push( new " << boundName << "( \"" << table->realName << "\", " << table->lowercaseName << ", " << str( boost::format( "0x%X" ) % hash ) << " ) );\n";
			}
		}
		file << indention << indention << "}\n";

		//function which finds links targets:
		{
			file << indention << indention << "\n";
			file << indention << indention << "/** Looks for link target.*/\n";
			file << indention << indention << "public static function " << findLinkTargetFunctionName << "( where:Object, id:int ): " << everyonesParentName << "\n";
			file << indention << indention << "{\n";
			file << indention << indention << indention << "for each ( var some:Object in where )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "if ( some.id == id )\n";
			file << indention << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << indention << "return some as " << everyonesParentName << ";\n";
			file << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "return null;\n";
			file << indention << indention << "}\n";
		}


		//body close:
		file << indention << "}\n";

		//package close:
		file << "}\n\n";
	}

	//link, count, everyones parent, bound:
	{
		std::string linkFileName = str( boost::format( "%s/%s.as" ) % targetFolder % linkName );
		std::ofstream linkFile( linkFileName.c_str() );
		if ( linkFile.fail() )
		{
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % linkFileName );
			return false;
		}
		
		std::string countFileName = str( boost::format( "%s/%s.as" ) % targetFolder % countName );
		std::ofstream countFile( countFileName.c_str() );
		if ( countFile.fail() )
		{
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % countFileName );
			return false;
		}

		std::string everyonesParentFileName = str( boost::format( "%s/%s.as" ) % targetFolder % everyonesParentName );
		std::ofstream everyonesParentFile( everyonesParentFileName.c_str() );
		if ( everyonesParentFile.fail() )
		{
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % everyonesParentFile );
			return false;
		}

		std::string boundFileName = str( boost::format( "%s/%s.as" ) % targetFolder % boundName );
		std::ofstream boundFile( boundFileName.c_str() );
		if ( boundFile.fail() )
		{
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % boundFileName );
			return false;
		}
		
		//file's head:
		{
			//intro:
			linkFile << explanation;
			countFile << explanation;
			everyonesParentFile << explanation;
			boundFile << explanation;

			//doxygen:
			linkFile << doxygen;
			countFile << doxygen;
			everyonesParentFile << doxygen;
			boundFile << doxygen;
			linkFile << doxygen_cond;
			countFile << doxygen_cond;
			everyonesParentFile << doxygen_cond;
			boundFile << doxygen_cond;

			//package:
			const std::string similarPackage = str( boost::format( "package %s\n{\n" ) % overallNamespace );
			linkFile << similarPackage;
			countFile << similarPackage;
			everyonesParentFile << similarPackage;
			boundFile << similarPackage;

			//imports:
			linkFile << indention << "import " << overallNamespace << "." << countName << ";\n";
			countFile << indention << "import " << overallNamespace << "." << everyonesParentName << ";\n";

			//doxygen:
			linkFile << indention << doxygen;
			countFile << indention << doxygen;
			everyonesParentFile << indention << doxygen;
			boundFile << indention << doxygen;
			linkFile << indention << doxygen_endcond;
			countFile << indention << doxygen_endcond;
			everyonesParentFile << indention << doxygen_endcond;
			boundFile << indention << doxygen_endcond;

			linkFile << indention << "\n" << indention << "\n";
			countFile << indention << "\n" << indention << "\n";
			everyonesParentFile << indention << "\n" << indention << "\n";
			boundFile << indention << "\n" << indention << "\n";
		}

		//name:
		{
			linkFile << indention << "/** Field that incapsulates all objects and counts to which object was linked*/\n";
			countFile << indention << "/** Link to single object which holds object's count along with object's pointer.*/\n";
			everyonesParentFile << indention << "/** All design types inherited from this class to permit objects storage within single array.*/\n";
			boundFile << indention << "/** Relation of tables names to it's data.*/\n";
			linkFile << indention << "public class " << linkName << "\n";
			countFile << indention << "public class " << countName << "\n";
			everyonesParentFile << indention << "public class " << everyonesParentName << "\n";
			boundFile << indention << "public class " << boundName << "\n";
		}

		//body open:
		linkFile << indention << "{\n";
		countFile << indention << "{\n";
		everyonesParentFile << indention << "{\n";
		boundFile << indention << "{\n";

		//fields:
		//link-specific:
		linkFile << indention << indention << "/** All linked objects. Defined within constructor.*/\n";
		linkFile << indention << indention << "public var links:Vector.<" << countName << "> = new Vector.<" << countName << ">;\n\n";
		//count-specific:
		countFile << indention << indention << str(boost::format("/** Linked object. Defined within constructor.*/\n"));
		countFile << indention << indention << str(boost::format("public var object:%s;\n\n") % everyonesParentName);
		countFile << indention << indention << str(boost::format("/** Linked objects count. If count was not specified within XLS then 1. Interpretation depends on game logic.*/\n"));
		countFile << indention << indention << str(boost::format("public var count:int;\n\n"));
		//everyones-parent specific:
		everyonesParentFile << indention << indention << str(boost::format("/** Any data which can be set by end-user.*/\n"));
		everyonesParentFile << indention << indention << str(boost::format("public var __opaqueData:*;\n"));
		everyonesParentFile << indention << indention << str(boost::format("/** Name of tab where this type was defined.*/\n"));
		everyonesParentFile << indention << indention << str(boost::format("public var %s:String;\n") % tabName);
		everyonesParentFile << indention << indention << str(boost::format("/** Tab's short name to store it on server.*/\n"));
		everyonesParentFile << indention << indention << str(boost::format("public var __tabAlias:String;\n"));
		//bound specific:
		boundFile << indention << indention << str( boost::format( "/** Table's name without any modifications. Defined within constructor.*/\n" ) );
		boundFile << indention << indention << str( boost::format( "public var tableName:String;\n" ) );
		boundFile << indention << indention << str( boost::format( "/** Data objects of this table. Vector of objects inherited at least from \"%s\". Defined within constructor.*/\n" ) % everyonesParentName );
		boundFile << indention << indention << str( boost::format( "public var objects:Object;\n" ) );
		boundFile << indention << indention << str( boost::format( "/** Hash value from table's lowercase name to differentiate objects of this table from any other objects.*/\n" ) );
		boundFile << indention << indention << str( boost::format( "public var hash:uint;\n" ) );
		boundFile << indention << indention << "\n";


		//constructor:

		//initialization:
		linkFile << indention << indention << "/** All counts at once.*/\n";
		countFile << indention << indention << "/** Both pointer and count at construction.*/\n";
		boundFile << indention << indention << "/** Both name and array at construction.*/\n";
		linkFile << indention << indention << "public function " << linkName << "( links:Array )\n";
		countFile << indention << indention << "public function " << countName << str( boost::format( "( object:%s, count:int )\n" ) % everyonesParentName );
		boundFile << indention << indention << "public function " << boundName << str( boost::format( "( tableName:String, objects:Object, hash:uint )\n" ) );

		//definition:
		linkFile << indention << indention << "{\n";
		countFile << indention << indention << "{\n";
		boundFile << indention << indention << "{\n";

		//definition:
		//link-specific:
		linkFile << indention << indention << indention << "for ( var i:int = 0; i < links.length; ++i )\n";
		linkFile << indention << indention << indention << "{\n";
		linkFile << indention << indention << indention << indention << "this.links.push( links[ i ] as " << countName << " );\n";
		linkFile << indention << indention << indention << "}\n";
		//count-specific:
		countFile << indention << indention << indention << "this.object = object;\n";
		countFile << indention << indention << indention << "this.count = count;\n";
		//bound-specific:
		boundFile << indention << indention << indention << "this.tableName = tableName;\n";
		boundFile << indention << indention << indention << "this.objects = objects;\n";
		boundFile << indention << indention << indention << "this.hash = hash;\n";

		linkFile << indention << indention << "}\n";
		countFile << indention << indention << "}\n";
		boundFile << indention << indention << "}\n";


		//body close:
		linkFile << indention << "}\n";
		countFile << indention << "}\n";
		everyonesParentFile << indention << "}\n";
		boundFile << indention << "}\n";

		//package close:
		linkFile << "}\n\n";
		countFile << "}\n\n";
		everyonesParentFile << "}\n\n";
		boundFile << "}\n\n";
	}


	messenger << ( boost::format( "I: AS3 code successfully generated.\n" ) );

	return true;
}

