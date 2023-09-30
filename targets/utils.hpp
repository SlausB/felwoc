#include "ast/ast.h"
#include "output/messenger.h"
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
using namespace std;

bool parent_has_field( const Table * table, const Field * field );

void place_fields(
    const Table * table,
    vector< const Field * > & target
);

/** Orders table's fields by inheritance, where root has precedence.*/
template< typename T = Table >
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
);


/** Prints simple or multiline commentary with specified indention.*/
template< typename T = string >
std::string PrintCommentary( const string & indention, const std::string& commentary )
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

string print_string( const string & s );

bool IsText( const Field * field );
bool IsLink( const Field * field );


uint32_t fnv1aHash(const std::string& input);

void replace(
    string & where,
    const string & what,
    const string & with
);

template< typename T = Table >
auto cache_strings(
    const AST & ast,
    Messenger & messenger,
    const auto printer
) {
    map< string, int32_t > c;

    for ( const Table * t : ast.tables ) {
        for ( const auto & row : t->matrix ) {
            for ( const FieldData * data : row ) {
                if ( IsText( data->field ) ) {
                    c[ printer( messenger, data ) ] = 0;
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

uint32_t read_LEB128( auto & buffer ) {
	uint32_t message = 0;
	int8_t shift = 0;
	for ( int pos = 0; pos < 4; ++ pos )
	{
		uint8_t byte;
        buffer.read( (char*) & byte, 1 );
		
		message |= ( ( byte & 0x7F ) << shift );
		if ( ( byte & 0x80 ) == 0 )
		{
			break;
		}
		
		shift += 7;
	}
    return message;
}
void write_LEB128( auto & buffer, uint32_t message ) {
	do
	{
		uint8_t byte = message & 0x7F;
		message >>= 7;
		if ( message != 0 ) /* more bytes to come */
		{
			byte |= 0x80;
		}
        buffer.write( (const char*) & byte, 1 );
	}
	while ( message != 0 );
}

void write_string( auto & f, const string & s ) {
    write_LEB128( f, s.size() );
    f.write( s.c_str(), s.size() );
}

template< typename T = bool >
auto generate_hashes(
    const AST & ast,
    Messenger & messenger,
    bool & ok
) {
    //hash values generation to address tables:
    map< string, uint32_t > hashes;
    for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
    {
        Table* table = ast.tables[ tableIndex ];
        
        //std::hash< std::string > hash_function;
        const uint32_t hash = fnv1aHash( table->lowercaseName );

        //check that generated hash value doesn't collide with all already generated hash values:
        for ( map< string, uint32_t >::iterator it = hashes.begin(); it != hashes.end(); ++it ) {
            if ( hash == it->second ) {
                messenger << ( boost::format( "E: hash value generated from table's name \"%s\" collides with one generated from \"%s\". It happens once per ~100000 cases, so you're very \"lucky\" to face it. Change one of these table names' to something else to avoid this problem.\n" ) % table->lowercaseName % it->first );
                ok = false;
                return hashes;
            }
        }
        hashes[ table->lowercaseName ] = hash;
    }
    ok = true;
    return hashes;
}

template< typename T = bool >
auto make_class_names( const AST & ast, const auto postfix ) {
	map< const Table *, string > classNames;
	for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++tableIndex )
	{
		Table* table = ast.tables[ tableIndex ];
		string className = table->lowercaseName;
		std::transform( className.begin(), ++( className.begin() ), className.begin(), ::toupper );
		className.append( postfix );
		classNames[ table ] = className;
	}
    return classNames;
}

template< typename T = bool >
/** Tables that something can link AGAINST.*/
auto linkable_tables( const AST & ast ) {
    vector< Table * > linkable;
    for ( size_t tableIndex = 0; tableIndex < ast.tables.size(); ++ tableIndex )
    {
        Table* table = ast.tables[ tableIndex ];
        switch ( table->type )
        {
            case Table::MANY:
            case Table::MORPH:
            //these tables also can have links against something else (but nothing can link this table):
            case Table::PRECISE:
                linkable.push_back( table );
                break;
        }
    }
    return linkable;
}

auto indexOf( const auto & array, const auto & item ) {
    int32_t i = 0;
    for (
        auto it = begin( array );
        it != end( array );
        ++ it, ++ i
    ) {
        if ( item == * it ) {
            return i;
        }
    }
    return -1;
}
