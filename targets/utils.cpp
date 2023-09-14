#include "./utils.hpp"
#include <iostream>
using namespace std;

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

void replace(
    string & where,
    const string & what,
    const string & with
) {
    size_t pos = 0;
    while ( ( pos = where.find( what, pos ) ) != string::npos ) {
        where.replace( pos, what.length(), with );
        pos += with.length();
    }
}
