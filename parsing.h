

#ifndef PARSING_H
#define PARSING_H

#include <string>
#include <map>

#include "ast/ast.h"

#include "output/messenger.h"

#include "data_source/data_source.h"


#define MSG(message) messenger << message;

class Keywords
{
public:
	
	/** Returns -1 if keyword was NOT found, prints error with all possible keywords. Otherwise returns keyword's integer constant.*/
	int Match(Messenger& messenger, const std::string& tableName, const int rowIndex, const int columnIndex, const std::string& keyword) const;
	
	void Add(const std::string& keyword, const int match);

	/** Looks for key using value. Returns empty string if was not found. */
	std::string Find(Messenger& messenger, const int value);
	
	/** Возвращает список возможных вариантов, перечисленных через запятую.*/
	std::string PrintPossible() const;
	
	std::map<std::string, int> keywords;
};

/** Parsing utility functions.*/
class Parsing
{
public:
	
	Parsing();
	
	/** Compiles data source file.
	\return true if everything gone fine.*/
	bool ProcessSource(AST& ast, Messenger& messenger, DataSource* dataSource);
	
	/** Splits string, removes all spaces around each part and converts to lowercase.*/
	static std::vector<std::string> Detach(const std::string& what, const char* delimiters);
	
	/** Removes all spaces around each part.*/
	static void Cleanup(std::string& what);
	
	/** Predefined rows types.*/
	enum
	{
		ROW_TABLE_COMMENTARY,
		ROW_TABLE_TYPE,
		ROW_TOGGLES,
		ROW_FIELDS_TYPES,
		ROW_FIELDS_NAMES,
		ROW_FIELDS_COMMENTS,
		ROW_FIRST_DATA,
	};
	
	/** Predefined columns types.*/
	enum
	{
		COLUMN_ROWS_TOGGLES,	/**< Way to turn off rows from compilation.*/
		COLUMN_TABLE_VALUE,	/**< Where value (which type depends on row) is specified.*/
		COLUMN_MIN_COLUMN = 1,	/**< There has to be at least one column.*/
	};
	
	Keywords tableParamsKeywords;
	Keywords tableTypesKeywords;
	Keywords fieldKeywords;
	Keywords serviceFieldsKeywords;
	Keywords preciseTableKeywords;
};

#endif//#ifndef PARSING_H

