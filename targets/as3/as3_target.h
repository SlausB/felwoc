

#ifndef AS3_TARGET_H
#define AS3_TARGET_H

#include "../target_platform.h"

class AS3Target: public TargetPlatform
{
public:
	bool Generate(const AST& ast, Messenger& messenger, const boost::property_tree::ptree& config);
};

#endif//#ifndef AS3_TARGET_H

