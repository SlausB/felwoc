R"=====(/* This file is generated using the "xls2xj" program from XLS design file.
Bugs, issues or suggestions can be sent to SlavMFM@gmail.com */

package design;

import design.Count;
import openfl.Vector;


/** Field that incapsulates all objects and counts to which object was linked.*/
class Link
{
	/** All linked objects. Defined in constructor.*/
	public var links : Vector.< Count >;

	/** All counts at once.*/
	public function new( links : Vector.< Count > )
	{
		this.links = links
	}
}
)====="
