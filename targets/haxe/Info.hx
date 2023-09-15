R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs, issues or suggestions can be sent to SlavMFM@gmail.com */

package design;

import design.Bound;


/** All design types inherited from this class to permit objects storage within single array.*/
class Info
{
	/** Any data which can be set by end-user.*/
	public var __opaqueData : Any;
	/** Pointer to table's incapsulation object.*/
	public var __tabBound : Bound;

	public function new( __tabBound : Bound )
	{
		this.__tabBound = __tabBound;
	}
}
)====="
