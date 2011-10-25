

#include "utils.h"


void ForEachTable(Table* current, std::function<bool(Table*)> acceptor)
{
	for(;;)
	{
		if(acceptor(current) == false)
		{
			return;
		}
		
		if(current->parent == NULL)
		{
			return;
		}
		else
		{
			current = current->parent;
		}
	}
}

