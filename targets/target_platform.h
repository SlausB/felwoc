

#ifndef TARGET_PLATFORM_H
#define TARGET_PLATFORM_H

#include "../ast/ast.h"

#include "../output/messenger.h"

//configuration file:
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

/** Abstract target platform's code (or something else, such as JSON or XML files) generator.
All platform-specific generators must be derived from this class.*/
class TargetPlatform
{
public:
	/** Code generation entry-point.
	\param ast Whole code compiled from XLS file. Must NOT be modified within this function.
	\param config Loaded initialization file - can provide some settings.
	\return false if something gone wrong.*/
	virtual bool Generate( const AST& ast, Messenger& messenger, const boost::property_tree::ptree& config ) = 0;
};

#endif//#ifndef TARGET_PLATFORM_H

