
#include "ts_target.h"

#include <fstream>
#include <boost/filesystem.hpp>
#include <map>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <algorithm>

using namespace std;


namespace TS {

uint32_t fnv1aHash(const std::string& input) {
    const uint32_t FNV_OFFSET_BASIS = 2166136261u;
    const uint32_t FNV_PRIME = 16777619u;

    uint32_t hash = FNV_OFFSET_BASIS;

    for (char c : input) {
        hash ^= static_cast<uint32_t>(c);
        hash *= FNV_PRIME;
    }

    return hash;
}

std::string targetFolder;

const char* rootName = "Info";
	
const std::string indention = "	";

const char* linkName = "Link";
const char* countName = "Count";

const char* findLinkTargetFunctionName = "FindLinkTarget";

const char* allTablesName = "__all";

const std::string explanation = "/* This file is generated using the \"xls2xj\" program from XLS design file.\nBugs, issues or suggestions can be sent to SlavMFM@gmail.com */\n\n";

const char* packageName = "infos";

const char* incapsulationName = "Infos";

//appended to all class names:
const char* postfix = "Info";

//name of class which incapsulates table properties and array of it's objects:
const std::string boundName = "Bound";
const std::string lowerBoundName = "bound";
const std::string boundVariableName = "__bound";

const std::string tabBound = "__tabBound";

const std::string objectsField = "objects";
const std::string hashField = "hash";

const std::string stringsResolvingFunction = "ResolveStrings";
const std::string stringsInitingFunction = "InitStrings_";
const std::string objectsResolvingFunction = "ResolveObjects";
const std::string objectsInitingFunction = "InitObjects_";
const std::string linksResolvingFunction = "ResolveLinks";
const std::string linksInitingFunction = "InitLinks_";

const int STRINGS_PER_STEP = 1000;
const int OBJECTS_PER_STEP = 1000;
const int LINKS_PER_STEP   = 1000;

/** Time consumption part of initing processes.*/
const float STRINGS_PROGRESS_PART = 0.3;
const float OBJECTS_PROGRESS_PART = 0.5;
const float LINKS_PROGRESS_PART = 0.1;

/** Name of the function which will be called when everything's done.*/
const std::string onDone = "onDone";
/** Name of the function which used to inform initialization process fulfillness if specified.*/
const std::string progress = "progress";

void import(
    std::ofstream & o,
    const auto & target,
    const bool up = false
) {
    const auto from = up ? " from \"../" : " from \"./";
    o << "import " << target << from << target << "\"\n";
}
void obligatory_imports(
    std::ofstream & o,
    const bool up = false
) {
    const auto from = up ? " from \"../" : " from \"./";
    import( o, linkName, up );
    import( o, rootName, up );
    import( o, boundName, up );
}



bool IsLink( const Field * field )
{
	if ( field->type == Field::LINK ) {
		return true;
	}

	if ( field->type == Field::INHERITED ) {
		if ( ( ( InheritedField* )field )->parentField->type == Field::LINK ) {
			return true;
		}
	}

	return false;
}
bool IsText( const Field * field ) {
	if ( field->type == Field::TEXT ) {
		return true;
	}

	if ( field->type == Field::INHERITED ) {
		if ( ( ( InheritedField* )field )->parentField->type == Field::TEXT ) {
			return true;
		}
	}

	return false;
}

/** Returns empty string on some error.*/
std::string PrintType( Messenger& messenger, const Field* field )
{
	switch ( field->type ) {
        case Field::INHERITED: {
            InheritedField* inheritedField = ( InheritedField* ) field;
            return PrintType( messenger, inheritedField->parentField ); }

        case Field::SERVICE: {
            ServiceField* serviceField = (ServiceField*)field;
            switch(serviceField->serviceType) {
                case ServiceField::ID:
                    return "number";

                default:
                    messenger.error( boost::format( "E: TS: PROGRAM ERROR: PrintType(): service type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % serviceField->serviceType );
                    return std::string();
            } }
            break;

        case Field::TEXT:
            return "string";

        case Field::FLOAT:
            return "number";

        case Field::INT:
            return "number";

        case Field::LINK:
            return rootName;

        case Field::BOOL:
            return "boolean";

        case Field::ARRAY:
            return "Array< number >";

        
        default:
            messenger.error( boost::format( "E: TS: PROGRAM ERROR: PrintType(): type = %d is undefined. Returning empty string. Refer to software supplier.\n") % field->type );
	}

	return std::string();
}

string print_string( const string & s ) {
    std::string result = "\"";
    //escape:
    for ( size_t i = 0; i < s.size(); ++i )
    {
        switch ( s[ i ] ) {
            case '"':
                result.push_back( '\\' );
                break;

            case '\n':
                result.append( "\\n" );
                continue;
        }

        result.push_back( s[ i ] );
    }
    result.push_back( '"' );
    return result;
}
std::string PrintData( Messenger& messenger, const FieldData* fieldData )
{
	switch ( fieldData->field->type ) {
        case Field::INHERITED: {
            Inherited* inherited = ( Inherited* ) fieldData;
            return PrintData( messenger, inherited->fieldData ); }

        case Field::SERVICE: {
            Service* service = ( Service* ) fieldData;
            ServiceField* serviceField = ( ServiceField* ) fieldData->field;
            switch ( serviceField->serviceType )
            {
                case ServiceField::ID: {
                    Int* asInt = ( Int* ) service->fieldData;
                    return boost::lexical_cast<std::string>(asInt->value); }

                default:
                    messenger.error( boost::format( "E: TS: PROGRAM ERROR: PrintData(): service type %d is undefined. Refer to software supplier.\n" ) % serviceField->serviceType );
            } }
            break;

        case Field::TEXT: {
            Text* text = ( Text* ) fieldData;

            return print_string( text->text ); }

        case Field::FLOAT: {
            Float* asFloat = ( Float* ) fieldData;
            return boost::lexical_cast< std::string >( asFloat->value ); }

        case Field::INT: {
            Int* asInt = (Int*)fieldData;
            return boost::lexical_cast<std::string>(asInt->value); }

        case Field::LINK:
            //links will be connected later:
            return "NULL";

        case Field::BOOL: {
            Bool* asBool = (Bool*)fieldData;
            if ( asBool->value ) {
                return "true";
            }
            return "false"; }

        case Field::ARRAY: {
            Array* asArray = (Array*)fieldData;

            std::string arrayData = "[";
            for ( size_t i = 0; i < asArray->values.size(); ++i ) {
                if ( i > 0 ) {
                    arrayData.append( ", " );
                }

                arrayData.append( boost::lexical_cast< std::string >( asArray->values[ i ] ) );
            }
            arrayData.append( "]" );

            return arrayData; }

        
        default:
            messenger.error( boost::format( "E: TS: PROGRAM ERROR: PrintData(): type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % fieldData->field->type );
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
	for ( size_t i = 0; i < commentary.size(); ++i ) {
		if ( commentary[ i ] == '\n' ) {
			multiline = true;
			break;
		}
	}

	if ( multiline ) {
		result.append( "\n" );
		result.append( indention );

		bool lastIsEscape = false;
		for ( size_t i = 0; i < commentary.size(); ++i ) {
			result.push_back( commentary[ i ] );

			if ( commentary[ i ] == '\n' ) {
				result.append( indention );

				lastIsEscape = true;
			}
			else {
				lastIsEscape = false;
			}
		}
		if ( lastIsEscape == false ) {
			result.append( "\n" );
		}

		result.append( indention );
	}
	else {
		result.append( commentary );
		result.append( " " );
	}

	result.append( "*/\n" );

	return result;
}

void CloseSteppedFunction( std::ofstream & file, const int group, const auto name ) {
	//proceed initialization process:
	file << indention << indention << "\n";
	file << indention << indention << "this." << name << "( " << group << " );\n";

	file << indention << "}\n";
	file << indention << "\n";
}
void CloseObjectsInitingFunction( std::ofstream &file, const int group ) {
    CloseSteppedFunction( file, group, objectsResolvingFunction );
}

void CloseLinksInitingFunction( std::ofstream &file, const int group ) {
    CloseSteppedFunction( file, group, linksResolvingFunction );
}

void PrintResolvingFunction(
    std::ofstream & file,
    const string resolve_name,
    const string init_name,
    const int steps,
    const float progress_part,
    const string next
) {
    file << indention << "private " << resolve_name << "( step : number ) : void {\n";
    
    //inform progress if needed:
    file << indention << indention << "if ( this._" << progress << " ) {\n";
    file << indention << indention << indention << "this._" << progress << "( step / " << steps << " * " << progress_part << " );\n";
    file << indention << indention << "}\n";
    file << indention << indention << "\n";

    file << indention << indention << "this.postpone( () => {\n";

    //redirection itself:
    file << indention << indention << indention << "switch ( step ) {\n";
    for ( int i = 0; i < steps; ++i )
    {
        file << indention << indention << indention << indention << "case " << i << ":\n";
        file << indention << indention << indention << indention << indention << "this." << init_name << i << "();\n";
        file << indention << indention << indention << indention << indention << "break;\n";
        file << indention << indention << indention << indention << "\n";
    }
    file << indention << indention << indention << indention << "default:\n";
    file << indention << indention << indention << indention << indention << next << "\n";
    file << indention << indention << indention << indention << indention << "break;\n";
    file << indention << indention << indention << "}\n";

    //close the timer handler and initiate it:
    file << indention << indention << "} );\n";

    //close the function:
    file << indention << "}\n";
    file << indention << "\n";
}

} //namespace TS
using namespace TS;

bool parent_has_field( const Table * table, const Field * field ) {
    if ( ! table->parent ) {
        return false;
    }
    for ( const Field * pf : table->parent->fields ) {
        if ( pf->name == field->name ) {
            return true;
        }
    }
    return parent_has_field( table->parent, field );
}

void place_fields(
    const Table * table,
    vector< const Field * > & target
) {
    if ( table->parent ) {
        place_fields( table->parent, target );
    }
    for ( const Field * field : table->fields ) {
        if ( field->type == Field::INHERITED ) {
            continue;
        }
        target.push_back( field );
    }
}
/** Orders table's fields by inheritance, where root has precedence.*/
auto order_inheritance_wise( const AST & ast ) {
    map<
        const Table *,
        vector< const Field * >
    > ordered;

    for ( const Table * table : ast.tables ) {
        place_fields( table, ordered[ table ] );
    }

    return ordered;
}
int global_field_index(
    const Field * field,
    const vector< const Field * > & ordered
) {
    for ( int i = 0; i < (int)ordered.size(); ++ i ) {
        if ( ordered[ i ]->name == field->name ) {
            return i;
        }
    }
    return -1;
}

void TS_Target::LinkBeingConnected( std::ofstream &file ) {
	++_someLinkIndex;
	if ( _someLinkIndex % LINKS_PER_STEP == 0 ) {
		//close previous function if needed:
		if ( _linksSteps > 0 ) {
			CloseLinksInitingFunction( file, _linksSteps );
		}

		//start new function:
		file << indention << "private " << linksInitingFunction << _linksSteps << "() : void\n";
		file << indention << "{\n";

		++_linksSteps;
	}
}

auto cache_strings( const AST & ast, Messenger & messenger ) {
    map< string, int32_t > c;

    for ( const Table * t : ast.tables ) {
        for ( const auto & row : t->matrix ) {
            for ( const FieldData * data : row ) {
                if ( IsText( data->field ) ) {
                    c[ PrintData( messenger, data ) ] = 0;
                }
            }
        }
    }

    size_t i = 0;
    for ( auto & text : c ) {
        text.second = i;
        ++ i;
    }

    return c;
}


bool TS_Target::Generate(
    const AST& ast,
    Messenger& messenger,
    const boost::property_tree::ptree& config
) {
    const auto inheritance_ordered = order_inheritance_wise( ast );
    const auto strings_cache = cache_strings( ast, messenger );

	targetFolder = config.get< std::string >( "ts_target_folder", "./ts/code/design" );
	//get rid of last folder separator:
	if ( targetFolder.empty() == false ) {
		char last = targetFolder[ targetFolder.size() - 1 ];
		if ( last == '/' || last == '\\' ) {
			targetFolder.resize( targetFolder.size() - 1 );
		}
	}

	boost::filesystem::create_directories( targetFolder );
	boost::filesystem::create_directory( targetFolder + "/infos" );

	//make class names:
	std::map< const Table *, std::string > classNames;
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

		std::string fileName = str( boost::format( "%s/infos/%s.ts" ) % targetFolder % classNames[ table ] );
		std::ofstream file( fileName, std::ios_base::out );
		if ( file.fail() ) {
			messenger << ( boost::format( "E: TS: file \"%s\" was NOT opened.\n" ) % fileName );
			return false;
		}

		//file's head:
		{
			//intro:
			file << explanation;

			//imports:
            obligatory_imports( file, true );
			file << "\n";
            if ( table->parent ) {
                import( file, classNames[ table->parent ] );
            }

			file << "\n";
			file << "\n";
		}

		//name:
		{
			file << PrintCommentary( "", table->commentary );

			std::string parentName = table->parent == NULL ? rootName : classNames[ table->parent ];
			file << str( boost::format( "export default class %s extends %s\n" ) % classNames[ table ] % parentName );
		}

		//body open:
		file << "{\n";

		//fields:
		for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex )
		{
			Field* field = table->fields[ fieldIndex ];

			if ( field->type == Field::INHERITED ) {
				continue;
			}

			//commentary:
			file << PrintCommentary( indention, field->commentary );

			//field:

			const std::string fieldTypeName = PrintType( messenger, field );
			if ( fieldTypeName.empty() ) {
				return false;
			}
			file << indention << "public ";
			//links within tables of type "precise" will be defined the same way as all other links:
			if ( table->type == Table::PRECISE ) {
				file << "static ";
			}
			file << field->name << " : ";
			if ( IsLink( field ) ) {
				file << linkName;
			}
			else {
				file << fieldTypeName;
			}

			if ( table->type == Table::PRECISE ) {
				if ( ! IsLink( field ) ) {
					file << " = " << PrintData( messenger, table->matrix[ 0 ][ fieldIndex ] );
				}
			}

			file << ";\n";


			file << indention << "\n";
		}

        //muting inherited constructor:
        if ( table->type == Table::PRECISE ) {
            file << indention << "constructor() {\n";
            file << indention << indention << "super( undefined as unknown as " << boundName << " )\n";
            file << indention << "}\n";
        }

		//constructor:
		if ( table->type != Table::PRECISE && table->type != Table::SINGLE ) {
			//declaration:
			file << indention << "/** All (including inherited) fields (excluding links) are defined here. To let define classes only within it's classes without inherited constructors used undefined arguments.*/\n";
			file << indention << "constructor(\n";

            //first param is always tabBound:
            file << indention << indention << tabBound << " : " << boundName << ",\n";
            //rest params:
			for ( const Field * field : inheritance_ordered.at( table ) ) {
				if ( IsLink( field ) ) {
					continue;
				}
	
				file << indention << indention << field->name << " : " << PrintType( messenger, field ) << ",\n";
			}
            //close function args and open it's definition block:
            file << indention << ") {\n";
		

			//definition:

            //calling super() with at least tabBound and all it's fields:
			file << indention << indention << "super( " << tabBound << ", ";
            if ( table->parent ) {
                for ( const Field * field : inheritance_ordered.at( table->parent ) ) {
                    if ( IsLink( field ) ) {
                        continue;
                    }
                    file << field->name << ", ";
                }
            }
            file << ")\n";

			for ( Field * field : table->fields ) {
                if ( field->type == Field::INHERITED ) {
                    continue;
                }
                //muting TypeScript because we promise to assign values later on:
				if ( IsLink( field ) ) {
                    file << indention << indention << "//@ts-ignore\n";
                    file << indention << indention << "this." << field->name << " = undefined" << "\n";
				}
                else {
				    file << indention << indention << "this." << field->name << " = " << field->name << "\n";
                }
			}
			file << indention << "}\n";
		}
		

		//body close:
		file << "}\n";
	}

	//incapsulation:
	{
		//hash values generation to address tables:
		std::map< std::string, uint32_t > hashes;
		for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
		{
			Table* table = ast.tables[ tableIndex ];
			
            //std::hash< std::string > hash_function;
            const uint32_t hash = fnv1aHash( table->lowercaseName );

			//check that generated hash value doesn't collide with all already generated hash values:
			for ( std::map< std::string, uint32_t >::iterator it = hashes.begin(); it != hashes.end(); ++it ) {
				if ( hash == it->second ) {
					messenger << ( boost::format( "E: TS: hash value generated from table's name \"%s\" collides with one generated from \"%s\". It happens once per ~100000 cases, so you're very \"lucky\" to face it. Change one of these table names' to something else to avoid this problem.\n" ) % table->lowercaseName % it->first );
					return false;
				}
			}
			hashes[ table->lowercaseName ] = hash;
		}

		std::string fileName = str( boost::format( "%s/%s.ts" ) % targetFolder % incapsulationName );
		std::ofstream file( fileName.c_str() );
		if ( file.fail() ) {
			messenger << ( boost::format( "E: TS: file \"%s\" was NOT opened.\n" ) % fileName );
			return false;
		}
		
		//file's head:
		{
			//intro:
			file << explanation;

			//imports:
			{
                obligatory_imports( file );
                file << "import " << countName << " from \"./" << countName << "\"\n";
				file << "\n";

				//infos:
				bool once = false;
				for (
                    std::map< const Table *, std::string >::const_iterator it = classNames.begin();
                    it != classNames.end();
                    ++ it
                ) {
					file << "import " << it->second << " from \"" << "./" << packageName << "/" << it->second << "\"\n";
					once = true;
				}
				if ( once ) {
					file << "\n";
				}
			}

			file << "\n";
			file << "\n";
		}

		//name:
		{
			file << "/** Data definition.*/\n";
			file << "export default class " << incapsulationName << "\n";
		}

		//body open:
		file << "{\n";

		//fields:
		for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
		{
			Table* table = ast.tables[ tableIndex ];

			if ( table->type == Table::VIRTUAL ) {
				continue;
			}

			//commentary:
			file << PrintCommentary( indention, table->commentary );

			//as field:
			file << indention << "public " << table->lowercaseName;
			switch ( table->type )
			{
                case Table::MANY:
                //for now there are no differences with MANY:
                case Table::MORPH:
                    file << " : " << classNames[ table ] << " [] = new Array< " << classNames[ table ] << " >( " << table->matrix.size() << " )\n";
                    break;

                case Table::PRECISE:
                //for now there are no differences with PRECISE:
                case Table::SINGLE:
                    //such tables must be created to be able to define links later:
                    file << " : " << classNames[ table ] << " = new " << classNames[ table ] << ";\n";
                    break;

                default:
                    messenger.error( boost::format( "E: TS: table of type %d with name \"%s\" was NOT implemented. Refer to software supplier.\n" ) % table->type % table->realName );
                    return false;
			}

			file << indention << "\n";
		}
		//all:
		file << indention << "/** Array of all arrays. Each element is of type Vector.< " << rootName << " > */\n";
		file << indention << "public " << allTablesName << " : " << boundName << " [] = new Array< " << boundName << " >;\n";
		file << indention << "\n";

		//resulting function:
		file << indention << "/** Will be called when everything done.*/\n";
		file << indention << "private _" << onDone << " : Function\n";
		file << indention << "\n";

		//progress informing function:
		file << indention << "/** Called to inform process completeness state from 0 to 1 if specified.*/\n";
		file << indention << "private _" << progress << " : Function | undefined = undefined\n";
		file << indention << "\n";
		
		//bounds declaration:
		file << indention << "/** Will be created and temporarily used during each table objects creation: for each table will be created it's own.*/\n";
		file << indention << "private " << boundVariableName << " : " << boundName << "\n";
		file << indention << "\n";

		//cached empty link:
		file << indention << "/** There are really much of such: let's cache it.*/\n";
		file << indention << "private _emptyLink : " << linkName << " = new " << linkName << "( [ ] )\n";
		file << indention << "\n";

        //strings cache:
        file << indention << "/** Strings cache to reuse frequently reappearing literals.*/\n";
        file << indention << "private _c : string [] = new Array< " << strings_cache.size() << " >\n";
		file << indention << "\n";

		//constructor:
		//initialization:
		file << indention << "/** All data definition.\n";
		file << indention << "\\param " << onDone << " function() : void Called when asynchronous initialization is done.\n";
		file << indention << "\\param " << progress << " function( progress : number ) : void Used to inform about initialization progress if specified. 0 -> 1 where 1 is fully initialized.*/\n";
		file << indention << "constructor( " << onDone << " : ()=>any, " << progress << " : ((p:number)=>any) | undefined = undefined  )\n";
		//definition:
		file << indention << "{\n";

		//remembering resulting function:
		file << indention << indention << "this._" << onDone << " = " << onDone << ";\n";
		file << indention << indention << "this._" << progress << " = " << progress << ";\n";
		file << indention << indention << "\n";

		//initiate initing process:
		file << indention << indention << "this." << stringsResolvingFunction << "( 0 )\n";

		//close the constructor:
		file << indention << "}\n";
		file << indention << "\n";

        //strings cache initing functions:
        int stringsSteps = 0;
        int someCachedIndex = -1;
        for ( const auto & c : strings_cache ) {
            ++ someCachedIndex;

            if ( someCachedIndex % STRINGS_PER_STEP == 0 ) {
                //close previous function:
                if ( stringsSteps > 0 ) {
                    CloseSteppedFunction( file, stringsSteps, stringsResolvingFunction );
                }

                //start new function:
                file << indention << "private " << stringsInitingFunction << stringsSteps << "() : void {\n";

                ++ stringsSteps;
            }

            file << indention << indention << "this._c[ " << c.second << " ] = " << c.first << "\n";
        }
        CloseSteppedFunction( file, stringsSteps, stringsResolvingFunction );
		file << indention << "\n";

		//objects initing functions which differ only by periods as different steps:
		int objectsSteps = 0;
		bool somethingOut = false;
		int someObjectIndex = -1;
		for ( const Table * table : ast.tables )
		{
			if ( table->type != Table::MANY && table->type != Table::MORPH ) {
				continue;
			}

			if ( somethingOut ) {
				file << indention << "\n";
				somethingOut = false;
			}

			//data itself:
			//switching ".push()" semantic to "[ 666 ] = ...":
			int pushingIndex = -1;
			for (
                std::vector< std::vector< FieldData* > >::const_iterator row = table->matrix.begin();
                row != table->matrix.end();
                ++ row
            ) {
				++someObjectIndex;
				++pushingIndex;

				if ( someObjectIndex % OBJECTS_PER_STEP == 0 ) {
					//close previous function:
					if ( objectsSteps > 0 ) {
						CloseObjectsInitingFunction( file, objectsSteps );
					}

					//start new function:
					file << indention << "private " << objectsInitingFunction << objectsSteps << "() : void {\n";

					++objectsSteps;
				}

				//create and add bound here if it wasn't yet for currently initing table:
				if ( somethingOut == false && table->type == Table::MANY ) {
					file << indention << indention << "this." << boundVariableName << " = new " << boundName << "( \"" << table->realName << "\", this." << table->lowercaseName << ", " << str( boost::format( "0x%X" ) % hashes[ table->lowercaseName ] ) << " );\n";
					file << indention << indention << "this." << allTablesName << ".push( this." << boundVariableName << " );\n";
				}

				file << indention << indention << "this." << table->lowercaseName << "[ " << pushingIndex << " ] = new " << classNames.at( table ) << "( ";

				file << "this." << boundVariableName;

                vector< FieldData * > inh_reordered = * row;
                sort(
                    inh_reordered.begin(),
                    inh_reordered.end(),
                    [&]( const FieldData * a, const FieldData * b ) {
                        return
                            global_field_index( a->field, inheritance_ordered.at( table ) )
                            <
                            global_field_index( b->field, inheritance_ordered.at( table ) )
                        ;
                    }
                );

				for ( const FieldData * data : inh_reordered ) {
					if ( IsLink( data->field ) ) {
						continue;
					}

                    if ( IsText( data->field ) ) {
                        file << ", " << "this._c[ " << strings_cache.at( PrintData( messenger, data ) ) << " ]";
                    }
                    else {
					    file << ", " << PrintData( messenger, data );
                    }
				}

				file << " );\n";

				somethingOut = true;
			}
		}
		if ( somethingOut ) {
			CloseObjectsInitingFunction( file, objectsSteps );
		}
		file << indention << "\n";

		//connect links:
		_linksSteps = 0;
		_someLinkIndex = -1;
		{
			file << indention << "//connect links:\n";

			for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
			{
				Table* table = ast.tables[ tableIndex ];

				bool atLeastOne = false;

				//special for tables of type "precise":
				if ( table->type == Table::PRECISE ) {
					for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex ) {
						Field* field = table->fields[ fieldIndex ];

						if ( IsLink( field ) == false ) {
							continue;
						}

						if ( field->type != Field::LINK ) {
							messenger.error( boost::format( "E: TS: PROGRAM ERROR: linking precise: field's type = %d is undefined. Refer to software supplier.\n" ) % field->type );
							return false;
						}

						atLeastOne = true;

						LinkBeingConnected( file );

						Link* link = ( Link* ) ( table->matrix[ 0 ][ fieldIndex ] );

						file << indention << indention << classNames[ table ] << "." << field->name;
						if ( link->links.empty() ) {
							file << " = this._emptyLink;\n";
						}
						else {
							file << " = new " << linkName << "( [ ";
							for (
                                std::vector< Count >::const_iterator count = link->links.begin();
                                count != link->links.end();
                                ++ count
                            ) {
								if ( count != link->links.begin() ) {
									file << ", ";
								}

								file << "new " << countName << "( " << incapsulationName << "." << findLinkTargetFunctionName << "( " << "this." << count->table->lowercaseName << ", " << count->id << " ), " << count->count << " )";
							}
							file << " ] );\n";
						}
					}

					if ( atLeastOne ) {
						file << indention << indention << "\n";
					}

					//for table of type "precise" we're done:
					continue;
				}

				if ( table->type != Table::MANY && table->type != Table::MORPH ) {
					continue;
				}

				for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex ) {
					Field* field = table->fields[ fieldIndex ];

					if ( IsLink( field ) == false ) {
						continue;
					}

					atLeastOne = true;
				
					int rowIndex = 0;
					for (
                        std::vector< std::vector< FieldData* > >::const_iterator row = table->matrix.begin();
                        row != table->matrix.end(); ++row,
                        ++ rowIndex
                    ) {
						Link* link;
						if ( field->type == Field::LINK ) {
							link = ( Link* ) ( row->at( fieldIndex ) );
						}
						else if ( field->type == Field::INHERITED ) {
							link = ( Link* ) ( ( ( Inherited* ) ( row->at( fieldIndex ) ) )->fieldData );
						}
						else
						{
							messenger.error( boost::format( "E: TS: PROGRAM ERROR: linking: field's type = %d is undefined. Refer to software supplier.\n" ) % field->type );
							return false;
						}

						LinkBeingConnected( file );

						file << indention << indention << "this." << table->lowercaseName << "[ " << rowIndex << " ]." << field->name;
						if ( link->links.empty() ) {
							file << " = this._emptyLink;\n";
						}
						else {
							file << " = new " << linkName << "( [ ";
							for ( std::vector< Count >::const_iterator count = link->links.begin(); count != link->links.end(); ++count ) {
								if ( count != link->links.begin() ) {
									file << ", ";
								}

								file << "new " << countName << "( " << incapsulationName << "." << findLinkTargetFunctionName << "( " << "this." << count->table->lowercaseName << ", " << count->id << " ), " << count->count << " )";
							}
							file << " ] );\n";
						}
					}
				}

				if ( atLeastOne ) {
					file << indention << indention << indention << "\n";
				}
			}

			if ( _linksSteps > 0 ) {
				CloseLinksInitingFunction( file, _linksSteps );
			}
		}

        file << indention << "private postpone( f : ()=>any ) {\n";
        file << indention << indention << "setTimeout( f, 1 )\n";
        file << indention << "}\n";

        PrintResolvingFunction( file, stringsResolvingFunction, stringsInitingFunction, stringsSteps, STRINGS_PROGRESS_PART, str( boost::format( "this.%s(0)" ) % objectsResolvingFunction ) );
        PrintResolvingFunction( file, objectsResolvingFunction, objectsInitingFunction, objectsSteps, OBJECTS_PROGRESS_PART, str( boost::format( "this.%s(0)" ) % linksResolvingFunction ) );
        PrintResolvingFunction( file, linksResolvingFunction,   linksInitingFunction,   _linksSteps,  LINKS_PROGRESS_PART,   str( boost::format( "this._%s()" ) % onDone ) );

		//function which finds links targets:
		{
			file << indention << "\n";
			file << indention << "/** Looks for link target.*/\n";
			file << indention << "public static " << findLinkTargetFunctionName << "( where : " << rootName << " [], id : number ): " << rootName << " {\n";
			file << indention << indention << "for ( const some of where ) {\n";
			file << indention << indention << indention << "//@ts-ignore\n";
			file << indention << indention << indention << "if ( some.id == id ) {\n";
			file << indention << indention << indention << indention << "return some as " << rootName << ";\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << "}\n";
			file << indention << indention << "return undefined as unknown as " << rootName << "\n";
			file << indention << "}\n";
		}

		//function which safely obtains object of specified class from provided link:
		{
			file << indention << "\n";
			file << indention << "/** Obtain link at specified index of specified type.\n";
			file << indention << "\\param from Where to lookup for link.\n";
			file << indention << "\\param index Place of obtaining link from (including) zero. NULL will be returned and error printed if index is out of range.\n";
			file << indention << "\\param context Prefix for outputting error. Type something here to get understanding of where error occured when it will.*/\n";
			file << indention << "public static GetLink( from : Link, index : number, context : string ) : Count {\n";
			file << indention << indention << "if ( index >= from.links.length ) {\n";
			file << indention << indention << indention << "console.error( \"E: \" + context + \": requested link at index \" + index + \" but there are just \" + from.links.length + \" links. Returning undefined.\");\n";
			file << indention << indention << indention << "return undefined as unknown as Count\n";
			file << indention << indention << "}\n";
			file << indention << indention << "\n";
			file << indention << indention << "return from.links[ index ]\n";
			file << indention << "}\n";
		}

		//function to find info by it's values which are stored on server and transmitted through network:
		{
			file << indention << "\n";
			file << indention << "/** Use this function to restore Info when it is communicated over network.\n";
			file << indention << "\\param hash Hash value from table's name when looking Info is described.\n";
			file << indention << "\\param id Identificator within it's table.\n";
			file << indention << "\\return Found Info or null if nothing was found.*/\n";
			file << indention << "public FindInfo( hash : number, id : number ) : Info | undefined\n";
			file << indention << "{\n";
			file << indention << indention << "for ( const " << lowerBoundName << " of this." << allTablesName << " ) {\n";
			file << indention << indention << indention << "if ( hash == " << lowerBoundName << "." << hashField << " ) {\n";
			file << indention << indention << indention << indention << "for ( const info of " << lowerBoundName << "." << objectsField << " ) {\n";
            file << indention << indention << indention << indention << indention << "//@ts-ignore\n";
			file << indention << indention << indention << indention << indention << "if ( id == info.id ) {\n";
			file << indention << indention << indention << indention << indention << indention << "return info as " << rootName << ";\n";
			file << indention << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << indention << "}\n";
			file << indention << indention << indention << indention << "\n";
			file << indention << indention << indention << indention << "return undefined\n";
			file << indention << indention << indention << "}\n";
			file << indention << indention << "}\n";
			file << indention << indention << "return undefined\n";
			file << indention << "}\n";
		}


		//body close:
		file << "}\n";
	}

	//link, count, everyones parent, bound:
	{
		std::string linkFileName = str( boost::format( "%s/%s.ts" ) % targetFolder % linkName );
		std::ofstream linkFile( linkFileName.c_str() );
		if ( linkFile.fail() ) {
			messenger << ( boost::format( "E: TS: file \"%s\" was NOT opened.\n" ) % linkFileName );
			return false;
		}
		
		std::string countFileName = str( boost::format( "%s/%s.ts" ) % targetFolder % countName );
		std::ofstream countFile( countFileName.c_str() );
		if ( countFile.fail() ) {
			messenger << ( boost::format( "E: TS: file \"%s\" was NOT opened.\n" ) % countFileName );
			return false;
		}

		std::string rootFileName = str( boost::format( "%s/%s.ts" ) % targetFolder % rootName );
		std::ofstream rootFile( rootFileName.c_str() );
		if ( rootFile.fail() ) {
			messenger << ( boost::format( "E: TS: file \"%s\" was NOT opened.\n" ) % rootFileName );
			return false;
		}

		std::string boundFileName = str( boost::format( "%s/%s.ts" ) % targetFolder % boundName );
		std::ofstream boundFile( boundFileName.c_str() );
		if ( boundFile.fail() ) {
			messenger << ( boost::format( "E: TS: file \"%s\" was NOT opened.\n" ) % boundFileName );
			return false;
		}
		
		//file's head:
		{
			//intro:
			linkFile            << explanation;
			countFile           << explanation;
			rootFile            << explanation;
			boundFile           << explanation;

			//imports:
            import( linkFile, countName );
            import( countFile, rootName );
            import( rootFile, boundName );
            import( boundFile, rootName );

			linkFile            << "\n" << "\n";
			countFile           << "\n" << "\n";
			rootFile            << "\n" << "\n";
			boundFile           << "\n" << "\n";
		}

		//name:
		{
			linkFile            << "/** Field that incapsulates all objects and counts to which object was linked.*/\n";
			countFile           << "/** Link to single object which holds object's count along with object's pointer.*/\n";
			rootFile            << "/** All design types inherited from this class to permit objects storage within single array.*/\n";
			boundFile           << "/** Relation of tables names to it's data.*/\n";
			linkFile            << "export default class " << linkName            << "\n";
			countFile           << "export default class " << countName           << "\n";
			rootFile            << "export default class " << rootName            << "\n";
			boundFile           << "export default class " << boundName           << "\n";
		}

		//body open:
		linkFile            << "{\n";
		countFile           << "{\n";
		rootFile            << "{\n";
		boundFile           << "{\n";

		//fields:
		//link-specific:
		linkFile << indention << "/** All linked objects. Defined within constructor.*/\n";
		linkFile << indention << "public links : " << countName << " [] = []\n\n";
		//count-specific:
		countFile << indention << str( boost::format( "/** Linked object. Defined within constructor.*/\n" ) );
		countFile << indention << str( boost::format( "public object : %s\n" ) % rootName );
		countFile << indention << "\n";
		countFile << indention << str( boost::format( "/** Linked objects count. If count was not specified within XLS then 1. Interpretation depends on game logic.*/\n" ) );
		countFile << indention << str( boost::format( "public count : number\n" ) );
		countFile << indention << "\n";
		//everyones-parent specific:
		rootFile << indention << str( boost::format( "/** Any data which can be set by end-user.*/\n" ) );
		rootFile << indention << str( boost::format( "public __opaqueData : any | undefined = undefined\n" ) );
		rootFile << indention << str( boost::format( "/** Pointer to table's incapsulation object.*/\n" ) );
		rootFile << indention << str( boost::format( "public %s : %s\n" ) % tabBound % boundName );
		//bound specific:
		boundFile << indention << str( boost::format( "/** Table's name without any modifications. Defined within constructor.*/\n" ) );
		boundFile << indention << str( boost::format( "public tableName : string\n" ) );
		boundFile << indention << str( boost::format( "/** Data objects of this table. Vector of objects inherited at least from \"%s\". Defined within constructor.*/\n" ) % rootName );
		boundFile << indention << str( boost::format( "public %s : %s []\n" ) % objectsField % rootName );
		boundFile << indention << str( boost::format( "/** Hash value from table's lowercase name to differentiate objects of this table from any other objects.*/\n" ) );
		boundFile << indention << str( boost::format( "public %s : number\n" ) % hashField );
		boundFile << indention << "\n";


		//constructor:

		//initialization:
		linkFile  << indention << "/** All counts at once.*/\n";
		countFile << indention << "/** Both pointer and count at construction.*/\n";
		boundFile << indention << "/** Both name and array at construction.*/\n";
		linkFile  << indention << "constructor( links : " << countName << " [] )\n";
		countFile << indention << "constructor( object : " << rootName << ", count : number )\n";
        rootFile  << indention << "constructor( " << tabBound << " : " << boundName << " )\n";
		boundFile << indention << str( boost::format( "constructor( tableName : string, objects : %s [], hash : number )\n" ) % rootName );

		//definition:
		linkFile  << indention << "{\n";
		countFile << indention << "{\n";
        rootFile  << indention << "{\n";
		boundFile << indention << "{\n";

		//definition:
		//link-specific:
		linkFile  << indention << indention << "this.links = links\n";
		//count-specific:
		countFile << indention << indention << "this.object = object\n";
		countFile << indention << indention << "this.count = count\n";
        //root-specific:
        rootFile  << indention << indention << "this." << tabBound << " = " << tabBound << "\n";
		//bound-specific:
		boundFile << indention << indention << "this.tableName = tableName\n";
		boundFile << indention << indention << "this." << objectsField << " = objects\n";
		boundFile << indention << indention << "this." << hashField << " = hash\n";

		linkFile  << indention << "}\n";
		countFile << indention << "}\n";
		rootFile  << indention << "}\n";
		boundFile << indention << "}\n";


		//body close:
		linkFile            << "}\n";
		countFile           << "}\n";
		rootFile            << "}\n";
		boundFile           << "}\n";
	}


	messenger << ( boost::format( "I: TS code successfully generated.\n" ) );

	return true;
}

