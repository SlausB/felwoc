R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs, issues or suggestions can be sent to SlavMFM@gmail.com */

package design;

import Info from "./Info"


/** Link to single object which holds object's count along with object's pointer.*/
export default class Count
{
	/** Linked object. Defined within constructor.*/
	public object : Info
	
	/** Linked objects count. If count was not specified within XLS then 1. Interpretation depends on game logic.*/
	public count : number
	
	/** Both pointer and count at construction.*/
	constructor( object : Info, count : number )
	{
		this.object = object
		this.count = count
	}
}
)====="
