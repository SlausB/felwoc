﻿

#include "as3_target.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <map>

#include <boost/lexical_cast.hpp>

//#include "../../smhasher/MurmurHash3.h"
//trying to use std::hash instead:
#include <functional>

#include <algorithm>


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
const std::string boundName = "Bound";
const std::string lowerBoundName = "bound";
const std::string boundVariableName = "__bound";

const std::string tabBound = "__tabBound";

const std::string objectsField = "objects";
const std::string hashField = "hash";

const std::string objectsResolvingFunction = "ResolveObjects";
const std::string objectsInitingFunction = "InitObjects_";
const std::string linksResolvingFunction = "ResolveLinks";
const std::string linksInitingFunction = "InitLinks_";

const int OBJECTS_PER_STEP = 300;
const int LINKS_PER_STEP = 300;

/** Time consumption part of objects initing process.*/
const float OBJECTS_PROGRESS_PART = 0.5;

/** Name of the function which will be called when everything's done.*/
const std::string onDone = "onDone";
/** Name of the function which used to inform initialization process fulfillness if specified.*/
const std::string progress = "progress";




bool IsLink( Field* field )
{
	if ( field->type == Field::LINK )
	{
		return true;
	}

	if ( field->type == Field::INHERITED )
	{
		if ( ( ( InheritedField* )field )->parentField->type == Field::LINK )
		{
			return true;
		}
	}

	return false;
}

/** Returns empty string on some error.*/
std::string PrintType( Messenger& messenger, const Field* field )
{
	switch ( field->type )
	{
	case Field::INHERITED:
		{
			InheritedField* inheritedField = ( InheritedField* ) field;
			return PrintType( messenger, inheritedField->parentField );
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
				messenger.error( boost::format( "E: AS3: PROGRAM ERROR: PrintType(): service type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % serviceField->serviceType );
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
		messenger.error( boost::format( "E: AS3: PROGRAM ERROR: PrintType(): type = %d is undefined. Returning empty string. Refer to software supplier.\n") % field->type );
	}

	return std::string();
}

std::string PrintData( Messenger& messenger, const FieldData* fieldData )
{
	switch ( fieldData->field->type )
	{
	case Field::INHERITED:
		{
			Inherited* inherited = ( Inherited* ) fieldData;
			return PrintData( messenger, inherited->fieldData );
		}
		break;

	case Field::SERVICE:
		{
			Service* service = ( Service* ) fieldData;
			ServiceField* serviceField = ( ServiceField* ) fieldData->field;
			switch ( serviceField->serviceType )
			{
			case ServiceField::ID:
				{
					Int* asInt = ( Int* ) service->fieldData;
					return boost::lexical_cast<std::string>(asInt->value);
				}

			default:
				messenger.error( boost::format( "E: AS3: PROGRAM ERROR: PrintData(): service type %d is undefined. Refer to software supplier.\n" ) % serviceField->serviceType );
			}
		}
		break;

	case Field::TEXT:
		{
			Text* text = ( Text* ) fieldData;

			std::string result = "\"";
			//escape:
			for ( size_t i = 0; i < text->text.size(); ++i )
			{
				switch ( text->text[ i ] )
				{
				case '"':
					result.push_back( '\\' );
					break;

				case '\n':
					result.append( "\\n" );
					continue;
				}

				result.push_back( text->text[ i ] );
			}
			result.push_back( '"' );
			return result;
		}
		break;

	case Field::FLOAT:
		{
			Float* asFloat = ( Float* ) fieldData;
			return boost::lexical_cast< std::string >( asFloat->value );
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

void CloseObjectsInitingFunction( std::ofstream &file, const int group )
{
	//proceed initialization process:
	file << indention << indention << indention << "\n";
	file << indention << indention << indention << objectsResolvingFunction << "( " << group << " );\n";

	file << indention << indention << "}\n";
	file << indention << indention << "\n";
}

void CloseLinksInitingFunction( std::ofstream &file, const int group )
{
	//proceed initialization process:
	file << indention << indention << indention << "\n";
	file << indention << indention << indention << linksResolvingFunction << "( " << group << " );\n";

	file << indention << indention << "}\n";
	file << indention << indention << "\n";
}

void AS3Target::LinkBeingConnected( std::ofstream &file )
{
	++_someLinkIndex;
	if ( _someLinkIndex % LINKS_PER_STEP == 0 )
	{
		//close previous function if needed:
		if ( _linksSteps > 0 )
		{
			CloseLinksInitingFunction( file, _linksSteps );
		}

		//start new function:
		file << indention << indention << "private function " << linksInitingFunction << _linksSteps << "() : void\n";
		file << indention << indention << "{\n";

		++_linksSteps;
	}
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
		Table* table = ast.tables[ tableIndex ];
		std::string className = table->lowercaseName;
		std::transform( className.begin(), ++( className.begin() ), className.begin(), ::toupper );
		className.append( postfix );
		classNames[ table ] = className;
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
			//links within tables of type "precise" will be defined in the same way as all other links:
			if ( table->type == Table::PRECISE && field->type != Field::LINK )
			{
				file << "static const ";
			}
			else
			{
				file << "var ";
			}
			file << field->name << " : ";
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
			for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex )
			{
				Field* field = table->fields[ fieldIndex ];

				//links are defined after all initializations:
				if ( IsLink( field ) )
				{
					continue;
				}

				if ( once )
				{
					file << ", ";
				}
				once = true;

				const std::string fieldsTypeName = PrintType( messenger, field );
				if ( fieldsTypeName.empty() )
				{
					return false;
				}
				file << field->name << " : " << fieldsTypeName;
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
			file << indention << indention << indention << "this." << tabBound << " = args[ 0 ];\n";
			int argIndex = 1;
			for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex )
			{
				Field* field = table->fields[ fieldIndex ];

				if ( IsLink( field ) )
				{
					continue;
				}
	
				//file << indention << indention << indention << "this." << field->name << " = " << field->name << ";\n";
				file << indention << indention << indention << "this." << field->name << " = args[ " << argIndex << " ];\n";
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
		//hash values generation to address tables:
		std::map< std::string, uint32_t > hashes;
		for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
		{
			Table* table = ast.tables[ tableIndex ];
				
			/*uint32_t hash;
			MurmurHash3_x86_32( table->lowercaseName.c_str(), table->lowercaseName.size(), 0, &hash );*/
            std::hash< std::string > hash_function;
            const uint32_t hash = hash_function( table->lowercaseName );

			//check that generated hash value doesn't collide with all already generated hash values:
			for ( std::map< std::string, uint32_t >::iterator it = hashes.begin(); it != hashes.end(); ++it )
			{
				if ( hash == it->second )
				{
					messenger << ( boost::format( "E: AS3: hash value generated from table's name \"%s\" collides with one generated from \"%s\". It happens once per ~100000 cases, so you're very \"lucky\" to face it. Change one of these table names' to something else to avoid this problem.\n" ) % table->lowercaseName % it->first );
					return false;
				}
			}
			hashes[ table->lowercaseName ] = hash;
		}

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
				//to print some errors when they happen:
				file << indention << "import com.junkbyte.console.Cc;\n";
				file << indention << "import flash.utils.getQualifiedClassName;\n";
				file << indention << "import flash.utils.Timer;\n";
				file << indention << "import flash.events.TimerEvent;\n";
				file << indention << "\n";
				//ApathySync's obligatory:
				file << indention << "import " << overallNamespace << "." << linkName << ";\n";
				file << indention << "import " << overallNamespace << "." << everyonesParentName << ";\n";
				file << indention << "import " << overallNamespace << "." << boundName << ";\n";
				file << indention << "\n";

				//infos:
				bool once = false;
				for ( std::map< Table*, std::string >::const_iterator it = classNames.begin(); it != classNames.end(); ++it )
				{
					file << indention << "import " << overallNamespace << "." << packageName << "." << it->second << ";\n";
					once = true;
				}
				if ( once )
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
					file << " : Vector.< " << classNames[ table ] << " > = new Vector.< " << classNames[ table ] << " >( " << table->matrix.size() << ", true );\n";
				}
				break;

			case Table::PRECISE:
			//for now there are no differences with PRECISE:
			case Table::SINGLE:
				{
					//such tables must be created to be able to define links later:
					file << " : " << classNames[ table ] << " = new " << classNames[ table ] << ";\n";
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
		file << indention << indention << "public var " << allTablesName << " : Vector.< " << boundName << " > = new Vector.< " << boundName << " >;\n";
		file << indention << indention << "\n";

		//resulting function:
		file << indention << indention << "/** Will be called when everything done.*/\n";
		file << indention << indention << "private var _" << onDone << " : Function;\n";
		file << indention << indention << "\n";

		//progress informing function:
		file << indention << indention << "/** Called to inform process completeness state from 0 to 1 if specified.*/\n";
		file << indention << indention << "private var _" << progress << " : Function = null;\n";
		file << indention << indention << "\n";
		
		//bounds declaration:
		file << indention << indention << "/** Will be created and temporarely used during each table objects creation: for each table will be created it's own.*/\n";
		file << indention << indention << "private var " << boundVariableName << " : " << boundName << ";\n";
		file << indention << indention << "\n";

		//cached empty link:
		file << indention << indention << "/** There are really much of such: let's cache it.*/\n";
		file << indention << indention << "private var _emptyLink : " << linkName << " = new " << linkName << "( new < " << countName << " >[ ] );\n";
		file << indention << indention << "\n";

		//constructor:
		//initialization:
		file << indention << indention << "/** All data definition.\n";
		file << indention << indention << "\\param " << onDone << " function() : void Called when asynchronous initialization is done.\n";
		file << indention << indention << "\\param " << progress << " function( progress : int ) : void Used to inform about initialization progress if specified. 0 -> 1 where 1 is fully initialized.*/\n";
		file << indention << indention << "public function " << incapsulationName << "( " << onDone << " : Function, " << progress << " : Function = null )\n";
		//definition:
		file << indention << indention << "{\n";

		//remembering resulting function:
		file << indention << indention << indention << "_" << onDone << " = " << onDone << ";\n";
		file << indention << indention << indention << "_" << progress << " = " << progress << ";\n";
		file << indention << indention << indention << "\n";

		//initiate initing process:
		file << indention << indention << indention << objectsResolvingFunction << "( 0 );\n";

		//close the constructor:
		file << indention << indention << "}\n";
		file << indention << indention << "\n";

		//objects initing functions which differ only by periods as different steps:
		int objectsSteps = 0;
		bool somethingOut = false;
		int someObjectIndex = -1;
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
			//switching ".push()" semantic with "[ 666 ] = ...":
			int pushingIndex = -1;
			for ( std::vector< std::vector< FieldData* > >::const_iterator row = table->matrix.begin(); row != table->matrix.end(); ++row )
			{
				++someObjectIndex;
				++pushingIndex;

				if ( someObjectIndex % OBJECTS_PER_STEP == 0 )
				{
					//close previous function:
					if ( objectsSteps > 0 )
					{
						CloseObjectsInitingFunction( file, objectsSteps );
					}

					//start new function:
					file << indention << indention << "private function " << objectsInitingFunction << objectsSteps << "() : void\n";
					file << indention << indention << "{\n";

					++objectsSteps;
				}

				//create and add bound here if it wasn't yet for currently initing table:
				if ( somethingOut == false && table->type == Table::MANY )
				{
					file << indention << indention << indention << boundVariableName << " = new " << boundName << "( \"" << table->realName << "\", " << table->lowercaseName << ", " << str( boost::format( "0x%X" ) % hashes[ table->lowercaseName ] ) << " );\n";
					file << indention << indention << indention << allTablesName << ".push( " << boundVariableName << " );\n";
				}

				file << indention << indention << indention << table->lowercaseName << "[ " << pushingIndex << " ] = new " << classNames[ table ] << "( ";

				file << boundVariableName;

				for( std::vector< FieldData* >::const_iterator column = row->begin(); column != row->end(); ++column )
				{
					FieldData* fieldData = *column;

					if ( IsLink( fieldData->field ) )
					{
						continue;
					}

					file << ", " << PrintData( messenger, fieldData );
				}

				file << " );\n";

				somethingOut = true;
			}
		}
		if ( somethingOut )
		{
			CloseObjectsInitingFunction( file, objectsSteps );
		}
		file << indention << indention << indention << "\n";

		//connect links:
		_linksSteps = 0;
		_someLinkIndex = -1;
		{
			file << indention << indention << indention << "//connect links:\n";

			for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
			{
				Table* table = ast.tables[ tableIndex ];

				bool atLeastOne = false;

				//special for tables of type "precise":
				if ( table->type == Table::PRECISE )
				{
					for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex )
					{
						Field* field = table->fields[ fieldIndex ];

						if ( IsLink( field ) == false )
						{
							continue;
						}

						if ( field->type != Field::LINK )
						{
							messenger.error( boost::format( "E: AS3: PROGRAM ERROR: linking precise: field's type = %d is undefined. Refer to software supplier.\n" ) % field->type );
							return false;
						}

						atLeastOne = true;

						LinkBeingConnected( file );

						Link* link = ( Link* ) ( table->matrix[ 0 ][ fieldIndex ] );

						file << indention << indention << indention << table->lowercaseName << "." << field->name;
						if ( link->links.empty() )
						{
							file << " = _emptyLink;\n";
						}
						else
						{
							file << " = new " << linkName << "( new < " << countName << " >[ ";
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

					//for table of type "precise" we're done:
					continue;
				}

				if ( table->type != Table::MANY && table->type != Table::MORPH )
				{
					continue;
				}

				for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex )
				{
					Field* field = table->fields[ fieldIndex ];

					if ( IsLink( field ) == false )
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

						LinkBeingConnected( file );

						file << indention << indention << indention << table->lowercaseName << "[ " << rowIndex << " ]." << field->name;
						if ( link->links.empty() )
						{
							file << " = _emptyLink;\n";
						}
						else
						{
							file << " = new " << linkName << "( new < " << countName << " >[ ";
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
				}

				if ( atLeastOne )
				{
					file << indention << indention << indention << "\n";
				}
			}

			if ( _linksSteps > 0 )
			{
				CloseLinksInitingFunction( file, _linksSteps );
			}
		}

		//objects initing redirection function:
		{
			file << indention << indention << "private function " << objectsResolvingFunction << "( step : int ) : void\n";
			file << indention << indention << "{\n";
			
			//timer which will invoke initialization proceeding code:
			file << indention << indention << indention << "var timer : Timer = new Timer( 20, 1 );\n";
			file << indention << indention << indention << "\n";
			
			//inform progress if needed:
			file << indention << indention << indention << "if ( _" << progress << " != null )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "_" << progress << "( step / " << objectsSteps << " * " << OBJECTS_PROGRESS_PART << " );\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "\n";

			file << indention << indention << indention << "timer.addEventListener( TimerEvent.TIMER_COMPLETE, function( ... args ) : void\n";
			file << indention << indention << indention << "{\n";

			//redirection itself:
			file << indention << indention << indention << indention << "switch ( step )\n";
			file << indention << indention << indention << indention << "{\n";
			for ( int i = 0; i < objectsSteps; ++i )
			{
				file << indention << indention << indention << indention << indention << "case " << i << ":\n";
				file << indention << indention << indention << indention << indention << indention << objectsInitingFunction << i << "();\n";
				file << indention << indention << indention << indention << indention << indention << "break;\n";
				file << indention << indention << indention << indention << indention << "\n";
			}
			file << indention << indention << indention << indention << indention << "default:\n";
			file << indention << indention << indention << indention << indention << indention << linksResolvingFunction << "( 0 );\n";
			file << indention << indention << indention << indention << indention << indention << "break;\n";
			file << indention << indention << indention << indention << "}\n";

			//close the timer handler and initiate it:
			file << indention << indention << indention << "} );\n";
			file << indention << indention << indention << "\n";
			file << indention << indention << indention << "timer.start();\n";

			//close the function:
			file << indention << indention << "}\n";
			file << indention << indention << "\n";
		}

		//links connection redirection function:
		{
			file << indention << indention << "private function " << linksResolvingFunction << "( step : int ) : void\n";
			file << indention << indention << "{\n";

			//timer which will invoke initialization proceeding code:
			file << indention << indention << indention << "var timer : Timer = new Timer( 20, 1 );\n";
			file << indention << indention << indention << "\n";

			//inform progress if needed:
			file << indention << indention << indention << "if ( _" << progress << " != null )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "_" << progress << "( " << OBJECTS_PROGRESS_PART << " + ( step / " << _linksSteps << " * ( 1.0 - " << OBJECTS_PROGRESS_PART << " ) ) );\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "\n";

			file << indention << indention << indention << "timer.addEventListener( TimerEvent.TIMER_COMPLETE, function( ... args ) : void\n";
			file << indention << indention << indention << "{\n";

			//redirection itself:
			file << indention << indention << indention << indention << "switch ( step )\n";
			file << indention << indention << indention << indention << "{\n";
			for ( int i = 0; i < _linksSteps; ++i )
			{
				file << indention << indention << indention << indention << indention << "case " << i << ":\n";
				file << indention << indention << indention << indention << indention << indention << linksInitingFunction << i << "();\n";
				file << indention << indention << indention << indention << indention << indention << "break;\n";
				file << indention << indention << indention << indention << indention << "\n";
			}
			file << indention << indention << indention << indention << indention << "default:\n";
			file << indention << indention << indention << indention << indention << indention << "_" << onDone << "();\n";
			file << indention << indention << indention << indention << indention << indention << "break;\n";
			file << indention << indention << indention << indention << "}\n";

			//close the timer handler and initiate it:
			file << indention << indention << indention << "} );\n";
			file << indention << indention << indention << "\n";
			file << indention << indention << indention << "timer.start();\n";

			//close the function:
			file << indention << indention << "}\n";
		}
		
		//function which finds links targets:
		{
			file << indention << indention << "\n";
			file << indention << indention << "/** Looks for link target.*/\n";
			file << indention << indention << "public static function " << findLinkTargetFunctionName << "( where : Object, id : int ): " << everyonesParentName << "\n";
			file << indention << indention << "{\n";
			file << indention << indention << indention << "for each ( var some : Object in where )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "if ( some.id == id )\n";
			file << indention << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << indention << "return some as " << everyonesParentName << ";\n";
			file << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "return null;\n";
			file << indention << indention << "}\n";
		}

		//function which safely obtains object of specified class from provided link:
		{
			file << indention << indention << "\n";
			file << indention << indention << "/** Obtain link at specified index of specified type.\n";
			file << indention << indention << "\\param from Where to lookup for link.\n";
			file << indention << indention << "\\param index Place of obtaining link from (including) zero. NULL will be returned and error printed if index is out of range.\n";
			file << indention << indention << "\\param type Class of obtaining link. NULL will be returned and error printed if obtained link cannot be cast to specified class.\n";
			file << indention << indention << "\\param context Prefix for outputting error. Type something here to get understanding of where error occured when it will.*/\n";
			file << indention << indention << "public static function GetLink( from : Link, index : int, type : Class, context : String ) : Count\n";
			file << indention << indention << "{\n";
			file << indention << indention << indention << "if ( from == null || type == null || context == null )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "Cc.error( \"E: \" + context + \": some of params is null.\" );\n";
			file << indention << indention << indention << indention << "return null;\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "\n";
			file << indention << indention << indention << "if ( index >= from.links.length )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "Cc.error( \"E: \" + context + \": requested link at index \" + index + \" but there are just \" + from.links.length + \" links. Returning null.\");\n";
			file << indention << indention << indention << indention << "return null;\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "\n";
			file << indention << indention << indention << "var target : Object = from.links[ index ].object;\n";
			file << indention << indention << indention << "if ( ( target is type ) == false )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "Cc.error( \"E: \" + context + \": requested object of type \\\"\" + getQualifiedClassName( type ) + \"\\\", but object is of type \\\"\" + getQualifiedClassName( target ) + \"\\\" at index \" + index + \". Returning null.\" );\n";
			file << indention << indention << indention << indention << "return null;\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "\n";
			file << indention << indention << indention << "return from.links[ index ];\n";
			file << indention << indention << "}\n";
		}

		//function to find info by it's values which are stored on server and transmitted through network:
		{
			file << indention << indention << "\n";
			file << indention << indention << "/** Use this function to restore Info when it is transmitted through network.\n";
			file << indention << indention << "\\param hash Hash value from table's name when looking Info is described.\n";
			file << indention << indention << "\\param id Identificator within it's table.\n";
			file << indention << indention << "\\return Found Info or null if nothing was found.*/\n";
			file << indention << indention << "public function FindInfo( hash : uint, id : int ) : Info\n";
			file << indention << indention << "{\n";
			file << indention << indention << indention << "for each ( var " << lowerBoundName << " : " << boundName << " in " << allTablesName << " )\n";
			file << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << "if ( hash == " << lowerBoundName << "." << hashField << " )\n";
			file << indention << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << indention << "for each ( var info : Object in " << lowerBoundName << "." << objectsField << " )\n";
			file << indention << indention << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << indention << indention << "if ( id == info.id )\n";
			file << indention << indention << indention << indention << indention << indention << "{\n";
			file << indention << indention << indention << indention << indention << indention << indention << "return info as " << everyonesParentName << ";\n";
			file << indention << indention << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << indention << indention << "\n";
			file << indention << indention << indention << indention << indention << "return null;\n";
			file << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << indention << "return null;\n";
			file << indention << indention << "}\n";
		}


		//body close:
		file << indention << "}\n";

		//package close:
		file << "}\n\n\n\n";
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
			messenger << ( boost::format( "E: AS3: file \"%s\" was NOT opened.\n" ) % everyonesParentFileName );
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
		linkFile << indention << indention << "public var links : Vector.< " << countName << " > = new Vector.< " << countName << " >;\n\n";
		//count-specific:
		countFile << indention << indention << str( boost::format( "/** Linked object. Defined within constructor.*/\n" ) );
		countFile << indention << indention << str( boost::format( "public var object : %s;\n" ) % everyonesParentName );
		countFile << indention << indention << "\n";
		countFile << indention << indention << str( boost::format( "/** Linked objects count. If count was not specified within XLS then 1. Interpretation depends on game logic.*/\n" ) );
		countFile << indention << indention << str( boost::format( "public var count : int;\n" ) );
		countFile << indention << indention << "\n";
		//everyones-parent specific:
		everyonesParentFile << indention << indention << str( boost::format( "/** Any data which can be set by end-user.*/\n" ) );
		everyonesParentFile << indention << indention << str( boost::format( "public var __opaqueData : *;\n" ) );
		everyonesParentFile << indention << indention << str( boost::format( "/** Pointer to table's incapsulation object.*/\n" ) );
		everyonesParentFile << indention << indention << str( boost::format( "public var %s : %s;\n" ) % tabBound % boundName );
		//bound specific:
		boundFile << indention << indention << str( boost::format( "/** Table's name without any modifications. Defined within constructor.*/\n" ) );
		boundFile << indention << indention << str( boost::format( "public var tableName : String;\n" ) );
		boundFile << indention << indention << str( boost::format( "/** Data objects of this table. Vector of objects inherited at least from \"%s\". Defined within constructor.*/\n" ) % everyonesParentName );
		boundFile << indention << indention << str( boost::format( "public var %s : Object;\n" ) % objectsField );
		boundFile << indention << indention << str( boost::format( "/** Hash value from table's lowercase name to differentiate objects of this table from any other objects.*/\n" ) );
		boundFile << indention << indention << str( boost::format( "public var %s : uint;\n" ) % hashField );
		boundFile << indention << indention << "\n";


		//constructor:

		//initialization:
		linkFile << indention << indention << "/** All counts at once.*/\n";
		countFile << indention << indention << "/** Both pointer and count at construction.*/\n";
		boundFile << indention << indention << "/** Both name and array at construction.*/\n";
		linkFile << indention << indention << "public function " << linkName << "( links : Vector.< " << countName << " > )\n";
		countFile << indention << indention << "public function " << countName << str( boost::format( "( object : %s, count : int )\n" ) % everyonesParentName );
		boundFile << indention << indention << "public function " << boundName << str( boost::format( "( tableName : String, objects : Object, hash : uint )\n" ) );

		//definition:
		linkFile << indention << indention << "{\n";
		countFile << indention << indention << "{\n";
		boundFile << indention << indention << "{\n";

		//definition:
		//link-specific:
		linkFile << indention << indention << indention << "this.links = links;\n";
		//count-specific:
		countFile << indention << indention << indention << "this.object = object;\n";
		countFile << indention << indention << indention << "this.count = count;\n";
		//bound-specific:
		boundFile << indention << indention << indention << "this.tableName = tableName;\n";
		boundFile << indention << indention << indention << "this." << objectsField << " = objects;\n";
		boundFile << indention << indention << indention << "this." << hashField << " = hash;\n";

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

