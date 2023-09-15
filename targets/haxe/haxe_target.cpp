
#include "haxe_target.h"
#include "../utils.hpp"

#include <fstream>
#include <boost/filesystem.hpp>
#include <map>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <algorithm>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;


namespace Haxe {

string infos_text =
#include "./Infos.hx"
"";

string bound_text =
#include "./Bound.hx"
"";
string count_text =
#include "./Count.hx"
"";
string link_text =
#include "./Link.hx"
"";
string fosutils_text =
#include "./FosUtils.hx"
"";

std::string targetFolder;
	
const std::string indention = "    ";

const auto rootName = "Info";
const auto linkName = "Link";
const auto countName = "Count";
const auto boundName = "Bound";

//appended to all class names:
const char* postfix = "Info";

const std::string explanation = "/* This file is generated using the \"xls2xj\" program from XLS design file.\nBugs, issues or suggestions can be sent to SlavMFM@gmail.com */\n\n";

const std::string tabBound = "__tabBound";

void import(
    std::ofstream & o,
    const auto & target
) {
    o << "import design." << target << ";\n";
}
void obligatory_imports(
    std::ofstream & o,
    const bool up = false
) {
    import( o, linkName );
    import( o, rootName );
    import( o, boundName );
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
                    return "Int";

                default:
                    messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: PrintType(): service type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % serviceField->serviceType );
                    return std::string();
            } }
            break;

        case Field::TEXT:
            return "String";

        case Field::FLOAT:
            return "Float";

        case Field::INT:
            return "Int";

        case Field::LINK:
            return linkName;

        case Field::BOOL:
            return "Bool";

        case Field::ARRAY:
            return "Array< Float >";

        
        default:
            messenger.error( boost::format( "E: TS: PROGRAM ERROR: PrintType(): type = %d is undefined. Returning empty string. Refer to software supplier.\n") % field->type );
	}

	return std::string();
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
                    messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: PrintData(): service type %d is undefined. Refer to software supplier.\n" ) % serviceField->serviceType );
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
            return "null";

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
            messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: PrintData(): type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % fieldData->field->type );
	}

	return string();
}
/** Write data reading and initialization code.*/
std::string init_data( Messenger& messenger, const Field * field )
{
	switch ( field->type ) {
        case Field::INHERITED: {
            InheritedField * inherited = ( InheritedField * ) field;
            return init_data( messenger, inherited->parentField ); }
            break;

        case Field::SERVICE: {
            ServiceField* serviceField = ( ServiceField* ) field;
            switch ( serviceField->serviceType )
            {
                case ServiceField::ID: {
                    return "n()";

                default:
                    messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: init_data(): service type %d is undefined. Refer to software supplier.\n" ) % serviceField->serviceType );
            } } }
            break;

        case Field::TEXT:
            return "_c[ n() ]";

        case Field::FLOAT:
            return "f()";

        case Field::INT:
            return "n()";

        case Field::LINK:
            //links will be connected later:
            return "_e";

        case Field::BOOL:
            return "Bool( n() )";

        case Field::ARRAY:
            return "a()";
        
        default:
            messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: init_data(): type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % field->type );
	}

	return string();
}
/** Write actual data to binary output file.*/
void write_data(
    ofstream & f,
    Messenger& messenger,
    const FieldData* fieldData,
    const auto & strings_cache
) {
	switch ( fieldData->field->type ) {
        case Field::INHERITED: {
            Inherited* inherited = ( Inherited* ) fieldData;
            write_data( f, messenger, inherited->fieldData, strings_cache );
            break; }

        case Field::SERVICE: {
            Service* service = ( Service* ) fieldData;
            ServiceField* serviceField = ( ServiceField* ) fieldData->field;
            switch ( serviceField->serviceType )
            {
                case ServiceField::ID: {
                    Int* asInt = ( Int* ) service->fieldData;
                    write_LEB128( f, asInt->value ); }
                    break;

                default:
                    messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: init_data(): service type %d is undefined. Refer to software supplier.\n" ) % serviceField->serviceType );
                    break;
            } }
            break;

        case Field::TEXT: {
            Text* text = ( Text* ) fieldData;
            write_LEB128( f, strings_cache.at( print_string( text->text ) ) ); }
            break;

        case Field::FLOAT: {
            Float* asFloat = ( Float* ) fieldData;
            write_string( f, to_string( asFloat->value ) ); }
            break;

        case Field::INT: {
            Int* asInt = ( Int* ) fieldData;
            write_LEB128( f, asInt->value ); }
            break;

        case Field::LINK:
            //links will be connected later:
            break;

        case Field::BOOL: {
            Bool* asBool = (Bool*)fieldData;
            write_LEB128( f, int( asBool->value ) ); }
            break;

        case Field::ARRAY: {
            Array* asArray = (Array*)fieldData;
            write_LEB128( f, asArray->values.size() );
            for ( size_t i = 0; i < asArray->values.size(); ++i ) {
                write_string( f, to_string( asArray->values[ i ] ) );
            } }
            break;
        
        default:
            messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: init_data(): type = %d is undefined. Returning empty string. Refer to software supplier.\n" ) % fieldData->field->type );
            break;
	}
}

void pass_over(
    const auto & file_name,
    const auto & text,
    auto & messenger
) {
    const auto full_path = str( boost::format( "%s/%s.hx" ) % targetFolder % file_name );
    ofstream file( full_path.c_str() );
    if ( file.fail() ) {
        messenger << ( boost::format( "E: Haxe: file \"%s\" was NOT opened.\n" ) % full_path );
        return;
    }
    file << text;
}

} //namespace Haxe
using namespace Haxe;



bool Haxe_Target::Generate(
    const AST& ast,
    Messenger& messenger,
    const boost::property_tree::ptree& config
) {
    const auto inheritance_ordered = order_inheritance_wise( ast );
    const auto strings_cache = cache_strings(
        ast,
        messenger,
        [&]( auto messenger, auto data ) {
            return PrintData( messenger, data );
        }
    );

	targetFolder = config.get< std::string >( "haxe_target_folder", "./haxe/code/design" );
	//get rid of last folder separator:
	if ( targetFolder.empty() == false ) {
		char last = targetFolder[ targetFolder.size() - 1 ];
		if ( last == '/' || last == '\\' ) {
			targetFolder.resize( targetFolder.size() - 1 );
		}
	}

	boost::filesystem::create_directories( targetFolder );
	boost::filesystem::create_directory( targetFolder + "/infos" );
    
    std::string binary_name = str( boost::format( "%s/xls2xj.bin" ) % targetFolder );
    ofstream bin( binary_name.c_str(), ios::out | ios::binary );
    if ( bin.fail() ) {
        messenger << ( boost::format( "E: Haxe: file \"%s\" was NOT opened.\n" ) % binary_name );
        return false;
    }

    //writing strings cache:
    write_LEB128( bin, strings_cache.size() );
    for ( size_t i = 0; i < strings_cache.size(); ++ i ) {
        const auto & s = find_if(
            strings_cache.begin(),
            strings_cache.end(),
            [&]( const auto & p ) {
                return p.second == i;
            }
        );
        if ( s == strings_cache.end() ) {
			messenger << ( boost::format( "E: Haxe: string cache element \"%s\" was NOT resolved.\n" ) % i );
            return false;
        }
        const auto e = s->first;

        write_string( bin, e );
    }

	const auto classNames = make_class_names( ast, postfix );

	//generate classes:
	for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
	{
		Table* table = ast.tables[ tableIndex ];

		std::string fileName = str( boost::format( "%s/infos/%s.hx" ) % targetFolder % classNames.at( table ) );
		std::ofstream file( fileName, std::ios_base::out );
		if ( file.fail() ) {
			messenger << ( boost::format( "E: Haxe: file \"%s\" was NOT opened.\n" ) % fileName );
			return false;
		}

		//file's head:
		{
			//intro:
			file << explanation;

			//imports:
            obligatory_imports( file );
			file << "\n";
            if ( table->parent ) {
                import( file, classNames.at( table->parent ) );
            }

			file << "\n";
			file << "\n";
		}

		//name:
		{
			file << PrintCommentary( "", table->commentary );

			std::string parentName = table->parent == NULL ? rootName : classNames.at( table->parent );
			file << str( boost::format( "export default class %s extends %s\n" ) % classNames.at( table ) % parentName );
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
            file << indention << indention << "super( null )\n";
            file << indention << "}\n";
        }

		//constructor:
		if ( table->type != Table::PRECISE && table->type != Table::SINGLE ) {
			//declaration:
			file << indention << "/** All (including inherited) fields (excluding links) are defined here. To let define classes only within it's classes without inherited constructors used undefined arguments.*/\n";
			file << indention << "public function new(\n";

            //first param is always tabBound:
            file << indention << indention << tabBound << " : " << boundName << ",\n";
            //rest params:
			for ( const Field * field : inheritance_ordered.at( table ) ) {
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
                file << indention << indention << "this." << field->name << " = " << field->name << ";\n";
			}
			file << indention << "}\n";
		}
		

		//body close:
		file << "}\n";
	}
    
    //incapsulation:
    {
        bool ok;
        const auto hashes = generate_hashes( ast, messenger, ok );
        if ( ! ok ) {
            return false;
        }

        //imports:
        {
            //infos import:
            stringstream infos;
            for ( const auto it : classNames ) {
                infos << "import design." << it.second << ";\n";
            }
            replace( infos_text, "{INFOS}", infos.str() );
        }

		//fields:
        stringstream fields;
		for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
		{
			Table* table = ast.tables[ tableIndex ];

			if ( table->type == Table::VIRTUAL ) {
				continue;
			}

			//commentary:
			fields << PrintCommentary( indention, table->commentary );

			//as field:
			fields << indention << "public final " << table->lowercaseName;
			switch ( table->type )
			{
                case Table::MANY:
                //for now there are no differences with MANY:
                case Table::MORPH:
                    fields << " = new Vector< " << classNames.at( table ) << " >( " << table->matrix.size() << " );\n";
                    break;

                case Table::PRECISE:
                //for now there are no differences with PRECISE:
                case Table::SINGLE:
                    //such tables must be created to be able to define links later:
                    fields << " = new " << classNames.at( table ) << "();\n";
                    break;

                default:
                    messenger.error( boost::format( "E: Haxe: table of type %d with name \"%s\" was NOT implemented. Refer to software supplier.\n" ) % table->type % table->realName );
                    return false;
			}

			fields << indention << "\n";
		}
        replace( infos_text, "{FIELDS}", fields.str() );

        const auto linkable = linkable_tables( ast );
        replace( infos_text, "{BOUNDS}", to_string( linkable.size() ) );
        const auto type_index = [&]( const Table * table ) -> int32_t {
            for ( size_t i = 0; i < linkable.size(); ++ i ) {
                if ( linkable[ i ] == table ) {
                    return i;
                }
            }
            messenger << ( boost::format( "E: Haxe: type index for table \"%s\" was NOT resolved.\n" ) % table->lowercaseName );
            return -1;
        };

        stringstream bounds;
        for ( size_t i = 0; i < linkable.size(); ++ i ) {
            const auto & t = linkable[ i ];
            bounds << indention << indention << "__all[ " << i << " ] = b( " << print_string( t->realName ) << ", ";
            if ( t->type == Table::PRECISE ) {
                bounds << "v( " << t->lowercaseName << " )";
            }
            else {
                bounds << t->lowercaseName;
            }
            bounds << ", " << str( boost::format( "0x%X" ) % hashes.at( t->lowercaseName ) ) << " );\n";
        }
        replace( infos_text, "{INIT_BOUNDS}", bounds.str() );

        replace( infos_text, "{STRINGS}", to_string( strings_cache.size() ) );

        //objects spawning:
        {
            stringstream load_types;
            load_types << indention << indention << "switch ( type ) {\n";
            for ( size_t i = 0; i < linkable.size(); ++ i ) {
                const auto & table = linkable[ i ];
                //tables PRECISE initialized at construction when declared as fields:
                if ( table->type == Table::PRECISE ) {
                    continue;
                }

                //abstract initialization:
                load_types << indention << indention << indention << "case " << i << ":\n";
                load_types << indention << indention << indention << indention << table->lowercaseName << "[ index ] = new " << table->realName << "( r( " << i << " ), ";
                for ( const auto & field : inheritance_ordered.at( table ) ) {
                    load_types << init_data( messenger, field ) << ", ";
                }
                load_types << ");\n";
                load_types << indention << indention << indention << indention << "break;\n";
                
                //binary data:
                for ( const auto & row : table->matrix ) {
                    vector< FieldData * > inh_reordered = row;
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
                        write_data( bin, messenger, data, strings_cache );
                    }
                }
            }
            load_types << indention << indention << "}\n";
            replace( infos_text, "{LOAD_TYPES}", load_types.str() );
        }

        //links connections:
        {
            //fields initialization:
            stringstream link_type;
            link_type << indention << indention << "switch ( type ) {\n";
            for ( size_t i = 0; i < linkable.size(); ++ i ) {
                const auto & table = linkable[ i ];

                stringstream type_case;
                type_case << indention << indention << indention << "case " << i << ":\n";

                type_case << indention << indention << indention << indention << "switch ( field ) {\n";
                int32_t order = 0;
                for ( const auto & field : inheritance_ordered.at( table ) ) {
                    if ( IsLink( field ) ) {
                        type_case << indention << indention << indention << indention << indention << "case " << order << ":\n";
                        type_case << indention << indention << indention << indention << indention << indention << "cast( o, " << classNames.at( table ) << " )." << field->name << " = l;\n";
                        type_case << indention << indention << indention << indention << indention << indention << "break;\n";
                        order += 1;
                    }
                }
                type_case << indention << indention << indention << indention << "}\n";
                type_case << indention << indention << indention << indention << "break;\n";

                if ( order > 0 ) {
                    link_type << type_case.str();
                }
            }
            link_type << indention << indention << "}\n";
            replace( infos_text, "{LINK_TYPES}", link_type.str() );

            //binary data:
            int32_t links_count = 0;
            stringstream links_data;

            const auto link_field_index = [&]( const Table * table, Field * link ) {
                const auto & io = inheritance_ordered.at( table );
                int32_t result = 0;
                for ( size_t i = 0; i < io.size(); ++ i ) {
                    const auto & maybe_link = io.at( i );
                    if ( ! IsLink( io.at( i ) ) ) {
                        continue;
                    }
                    if ( maybe_link == link ) {
                        return result;
                    }
                    result += 1;
                }
                return -1;
            };

			for ( const auto & table : ast.tables ) {
                for ( size_t fieldIndex = 0; fieldIndex < table->fields.size(); ++fieldIndex ) {
                    Field* field = table->fields[ fieldIndex ];
					if ( IsLink( field ) == false ) {
						continue;
					}
					int rowIndex = 0;
					for (
                        std::vector< std::vector< FieldData* > >::const_iterator row = table->matrix.begin();
                        row != table->matrix.end(); ++row,
                        ++ rowIndex
                    ) {
                        for ( const FieldData * data : * row ) {
                            if ( ! IsLink( data->field ) ) {
                                continue;
                            }

                            const Link* link;
                            if ( field->type == Field::LINK ) {
                                link = ( Link* ) ( row->at( fieldIndex ) );
                            }
                            else if ( field->type == Field::INHERITED ) {
                                link = ( Link* ) ( ( ( Inherited* ) ( row->at( fieldIndex ) ) )->fieldData );
                            }
                            else
                            {
                                messenger.error( boost::format( "E: Haxe: PROGRAM ERROR: linking: field's type = %d is undefined. Refer to software supplier.\n" ) % field->type );
                                return false;
                            }

                            if ( link->links.empty() ) {
                                continue;
                            }

                            write_LEB128( links_data, type_index( table ) );
                            if ( table->type == Table::PRECISE ) {
                                write_LEB128( links_data, 0 );
                            }
                            else {
                                write_LEB128( links_data, rowIndex );
                            }
                            write_LEB128( links_data, link_field_index( table, field ) );
                            write_LEB128( links_data, link->links.size() );
                            for ( const auto & count : link->links ) {
                                write_LEB128( links_data, type_index( count.table ) );
                                write_LEB128( links_data, count.id );
                                write_LEB128( links_data, count.count );
                            }
                        }
                    }
                }
            }

            write_LEB128( bin, links_count );
            const auto s = links_data.str();
            bin.write( s.c_str(), s.size() );
        }
        
        //writing down:
        std::string fileName = str( boost::format( "%s/Infos.hx" ) % targetFolder );
        std::ofstream file( fileName.c_str() );
        if ( file.fail() ) {
            messenger << ( boost::format( "E: Haxe: file \"%s\" was NOT opened.\n" ) % fileName );
            return false;
        }
        file << infos_text;
    }
    
    pass_over( "Bound",    bound_text,    messenger );
    pass_over( "Count",    count_text,    messenger );
    pass_over( "Link",     link_text,     messenger );
    pass_over( "FosUtils", fosutils_text, messenger );


	messenger << ( boost::format( "I: Haxe code successfully generated.\n" ) );

	return true;
}

