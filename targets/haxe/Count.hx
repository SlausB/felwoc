R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs, issues or suggestions can be sent to SlavMFM@gmail.com */

package design;

import design.Info;


/** Link to single object which holds object's count along with object's pointer.*/
class Count
{
	/** Linked object. Defined within constructor.*/
	public var object : Info;
	
	/** Linked objects count. If count was not specified within XLS then 1. Interpretation depends on game logic.*/
	public var count : Float;
	
	/** Both pointer and count at construction.*/
	public function new( object : Info, count : Float ) {
		this.object = object;
		this.count = count;
	}
}
)====="
