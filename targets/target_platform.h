

#ifndef TARGET_PLATFORM_H
#define TARGET_PLATFORM_H

#include "../ast/ast.h"

#include "../output/messenger.h"

/** Abstract target platform's code (or something else, such as JSON or XML files) generator.
All platform-specific generators must be derived from this class.*/
class TargetPlatform
{
public:
	/** Code generation entry-point.
	\return false if something gone wrong.*/
	virtual bool Generate(const AST& ast, Messenger& messenger) = 0;
};

#endif//#ifndef TARGET_PLATFORM_H

