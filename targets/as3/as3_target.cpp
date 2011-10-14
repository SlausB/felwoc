

#include "as3_target.h"

bool AS3Target::Generate(const AST& ast, Messenger& messenger)
{
	messenger << boost::format("as3 as so generated.\n");
	return true;
}

