

#ifndef PARSING_H
#define PARSING_H

#include <string>

/** Parsing utility functions.*/
class Parsing
{
public:
	/** Splits string and removes all spaces around each part.*/
	static std::vector<std::string> Detach(const std::string what, const char* delimiters);
	
	/** Removes all spaces around each part.*/
	static Cleanup(std::string& what);
};

#endif//#ifndef PARSING_H

