

#ifndef AS3_TARGET_H
#define AS3_TARGET_H

#include "../target_platform.h"

class AS3Target: public TargetPlatform
{
public:
	bool Generate(const AST& ast, Messenger& messenger);
};

#endif//#ifndef AS3_TARGET_H

