﻿

#ifndef PARSING_H
#define PARSING_H

#include <string>

#include "ast.h"

#define FAIL(message) messenger << message; errorsCount++; return;
#define MSG(message) messenger << message;

class Keywords
{
public:
	
	/** Returns -1 if keyword was NOT found, prints error with all possible keywords. Otherwise returns keyword's integer constant.*/
	int Match(const std::string& context, const std::string& keyword) const;
	
	void Add(const std::string& keyword, const int match);
	
	/** Возвращает список возможных вариантов, перечисленных через запятую.*/
	std::string PrintPossible();
	
	std::map<std::string, int> keywords;
};

/** Parsing utility functions.*/
class Parsing
{
public:
	
	Parsing();
	
	/** Compiles XLS.
	\return true if everything is fine.*/
	bool ProcessXLS(const std::string& fileName);
	
	/** Splits string, removes all spaces around each part and converts to lowercase.*/
	static std::vector<std::string> Detach(const std::string& what, const char* delimiters);
	
	/** Removes all spaces around each part.*/
	static Cleanup(std::string& what);
	
private:
	Keywords tableParamsKeywords;
	Keywords tableTypesKeywords;
	Keywords fieldKeywords;
};

#endif//#ifndef PARSING_H

