

#ifndef AS3_TARGET_H
#define AS3_TARGET_H

#include "../target_platform.h"

class AS3Target: public TargetPlatform
{
public:
	bool Generate( const AST& ast, Messenger& messenger, const boost::property_tree::ptree& config );

private:
	/** Called just before any link connecting code is being written. Opens new links connecting function and closes previous one if needed.*/
	void LinkBeingConnected( std::ofstream &file );

	/** Index of currently connecting link to determin function creation need.*/
	int _someLinkIndex;
	/** How much links initing functions created.*/
	int _linksSteps;
};

#endif//#ifndef AS3_TARGET_H

