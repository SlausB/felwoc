
#include "./string_output.h"

StringOutput::StringOutput(std::string start)
{
	result	= start;
}

void StringOutput::out(const char* msg, const int type)
{
	result.append(msg);
}

