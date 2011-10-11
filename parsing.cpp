

#include "parsing.h"

#include <boost/algorithm/string.hpp>


static std::vector<std::string> Parsing::Detach(const std::string what, const char* delimiters)
{
	std::vector<std::string> strs;
	boost::split(strs, what, boost::is_any_of(delimiters));
	for(int i = 0; i < strs.size(); i++)
	{
		Cleanup(strs[i]);
	}
	return strs;
}

static Parsing::Cleanup(std::string& what)
{
	for(int i = 0; i < what.size(); i++)
	{
		if(what[i] != ' ')
		{
			what.erase(0, i);
			break;
		}
	}
	
	int postfix = 0;
	for(int i = what.size() - 1; i >= 0; i--)
	{
		if(what[i] != ' ')
		{
			what.erase(i, std::string::npos);
			break;
		}
	}
}

